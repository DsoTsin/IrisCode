#include "irisvfs.h"

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>

#include <memory>
#include <atomic>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <unordered_map>

static_assert(sizeof(ULONG) == sizeof(uint32_t), "");
static_assert(sizeof(DWORD) == sizeof(uint32_t), "");
static_assert(sizeof(by_handle_file_info) == sizeof(BY_HANDLE_FILE_INFORMATION), "");
static_assert(sizeof(find_stream_dataw) == sizeof(WIN32_FIND_STREAM_DATA), "");
static_assert(sizeof(find_dataw) == sizeof(WIN32_FIND_DATAW), "");
static_assert(sizeof(file_time) == sizeof(FILETIME), "");

namespace memfs {
// Memfs helpers
class memfs_helper {
public:
  static inline std::wstring GetFileName(const std::wstring& path) {
    std::size_t x = path.find_last_of('\\');
    return path.substr(x + 1);
  }

  static inline std::wstring GetParentPath(const std::wstring& path) {
    std::size_t x = path.find_last_of('\\');
    if (x == 0)
      return L"\\";
    return path.substr(0, x);
  }

  static inline LONGLONG FileTimeToLlong(const FILETIME& f) {
    return DDwLowHighToLlong(f.dwLowDateTime, f.dwHighDateTime);
  }

  static inline void LlongToFileTime(LONGLONG v, FILETIME& filetime) {
    LlongToDwLowHigh(v, (uint32_t&)filetime.dwLowDateTime, (uint32_t&)filetime.dwHighDateTime);
  }

  static inline LONGLONG DDwLowHighToLlong(const uint32_t& low, const uint32_t& high) {
    return static_cast<LONGLONG>(high) << 32 | low;
  }

  static inline void LlongToDwLowHigh(const LONGLONG& v, uint32_t& low, uint32_t& hight) {
    hight = v >> 32;
    low = static_cast<uint32_t>(v);
  }

  static const std::wstring DataStreamNameStr;
  // Remove the stream type from the filename
  // Stream type are not supported so we ignore / remove them.
  static inline void RemoveStreamType(std::wstring& filename) {
    // Remove $DATA stream if exist as it is the default / main stream.
    auto data_stream_pos = filename.rfind(DataStreamNameStr);
    if (data_stream_pos == (filename.length() - DataStreamNameStr.length()))
      filename = filename.substr(0, data_stream_pos);
    // TODO: Remove $INDEX_ALLOCATION & $BITMAP
  }

  // Return a pair containing for example for \foo:bar
  // first: filename: foo
  // second: alternated stream name: bar
  // If the filename do not contain an alternated stream, second is empty.
  static inline std::pair<std::wstring, std::wstring> GetStreamNames(const std::wstring& filename) {
    // real_fileName - foo or foo:bar
    const auto real_fileName = memfs_helper::GetFileName(filename);
    auto stream_pos = real_fileName.find(L":");
    // foo does not have alternated stream, return an empty alternated stream.
    if (stream_pos == std::string::npos)
      return std::pair<std::wstring, std::wstring>(real_fileName, std::wstring());

    // foo:bar has an alternated stream
    // return first the file name and second the file stream name
    // first: foo - second: bar
    const auto main_stream = real_fileName.substr(0, stream_pos);
    ++stream_pos;
    const auto alternate_stream
        = real_fileName.substr(stream_pos, real_fileName.length() - stream_pos);
    return std::pair<std::wstring, std::wstring>(main_stream, alternate_stream);
  }

  // Return the filename without any stream informations.
  // <filename>:<stream name>:<stream type>
  static inline std::wstring GetFileNameStreamLess(
      const std::wstring& filename,
      const std::pair<std::wstring, std::wstring>& stream_names) {
    auto file_name = memfs_helper::GetParentPath(filename);
    if (file_name != L"\\") {
      // return \ when filename is at root.
      file_name += L"\\";
    }
    file_name += stream_names.first;
    return file_name;
  }
};
// Safe class wrapping a Win32 Security Descriptor
struct security_informations : std::mutex {
  std::unique_ptr<std::byte[]> descriptor = nullptr;
  uint32_t descriptor_size = 0;

  security_informations() = default;
  security_informations(const security_informations&) = delete;
  security_informations& operator=(const security_informations&) = delete;

  void SetDescriptor(PSECURITY_DESCRIPTOR securitydescriptor) {
    if (!securitydescriptor)
      return;
    descriptor_size = GetSecurityDescriptorLength(securitydescriptor);
    descriptor = std::make_unique<std::byte[]>(descriptor_size);
    memcpy(descriptor.get(), securitydescriptor, descriptor_size);
  }
};

// Contains file time metadata from a node
// The information can safely be accessed from any thread.
struct filetimes {
  void reset() { lastaccess = lastwrite = creation = get_currenttime(); }

  static bool empty(const FILETIME* filetime) {
    return filetime->dwHighDateTime == 0 && filetime->dwLowDateTime == 0;
  }

  static LONGLONG get_currenttime() {
    FILETIME t;
    GetSystemTimeAsFileTime(&t);
    return memfs_helper::DDwLowHighToLlong(t.dwLowDateTime, t.dwHighDateTime);
  }

  std::atomic<LONGLONG> creation;
  std::atomic<LONGLONG> lastaccess;
  std::atomic<LONGLONG> lastwrite;
};

// Memfs file context
// Each file/directory on the memfs has his own filenode instance
// Alternated streams are also filenode where the main stream \myfile::$DATA
// has all the alternated streams (e.g. \myfile:foo:$DATA) attached to him
// and the alternated has main_stream assigned to the main stream filenode.
class filenode {
public:
  filenode(const std::wstring& filename,
           bool is_directory,
           uint32_t file_attr,
           const /*PDOKAN_IO_SECURITY_CONTEXT*/ void* security_context);

  filenode(const filenode& f) = delete;

  uint32_t read(LPVOID buffer, uint32_t bufferlength, LONGLONG offset);
  uint32_t write(LPCVOID buffer, uint32_t number_of_bytes_to_write, LONGLONG offset);

  const LONGLONG get_filesize();
  void set_endoffile(const LONGLONG& byte_offset);

  // Filename can during a move so we need to protect it behind a lock
  const std::wstring get_filename();
  void set_filename(const std::wstring& filename);

  // Alternated streams
  void add_stream(const std::shared_ptr<filenode>& stream);
  void remove_stream(const std::shared_ptr<filenode>& stream);
  std::unordered_map<std::wstring, std::shared_ptr<filenode>> get_streams();

  // No lock needed above
  std::atomic<bool> is_directory = false;
  std::atomic<uint32_t> attributes = 0;
  LONGLONG fileindex = 0;
  std::shared_ptr<filenode> main_stream;

  filetimes times;
  security_informations security;

private:
  filenode() = default;

  std::mutex _data_mutex;
  // _data_mutex need to be aquired
  std::vector<uint8_t> _data;
  std::unordered_map<std::wstring, std::shared_ptr<filenode>> _streams;

  std::mutex _fileName_mutex;
  // _fileName_mutex need to be aquired
  std::wstring _fileName;
};
// Memfs filenode storage
// There is only one instance of fs_filenodes per dokan mount
// as fs_filenodes describre the whole filesystem hierarchy context.
class fs_filenodes {
public:
  fs_filenodes();

  // Add a new filenode to the filesystem hierarchy.
  // The file will directly be visible on the filesystem.
  // An already processed GetStreamNames can optional be provided
  int add(const std::shared_ptr<filenode>& filenode,
          std::optional<std::pair<std::wstring, std::wstring>> stream_names);

  // Return the filenode linked to the filename if present.
  std::shared_ptr<filenode> find(const std::wstring& filename);

  // Return all filenode of the directory scope give in param.
  std::set<std::shared_ptr<filenode>> list_folder(const std::wstring& filename);

  // Remove filenode from the filesystem hierarchy.
  // If the filenode has alternated streams attached, they will also be removed.
  // If the filenode is a directory not empty, all sub filenode will be removed
  // recursively.
  void remove(const std::wstring& filename);
  void remove(const std::shared_ptr<filenode>& filenode);

  // Move the current filenode position to the new one in the filesystem
  // hierarchy.
  int move(const std::wstring& old_filename,
           const std::wstring& new_filename,
           uint8_t replace_if_existing);

  // Help - return a pair containing for example for \foo:bar
  // first: filename: foo
  // second: alternated stream name: bar
  // If the filename do not contain an alternated stream, second is empty.
  static std::pair<std::wstring, std::wstring> get_stream_names(std::wstring real_filename);

private:
  // Global FS FileIndex count.
  // Note: Alternated stream and main stream share the same FileIndex.
  std::atomic<LONGLONG> _fs_fileindex_count = 1;

  // Mutex need to be aquired when using fileNodes / directoryPaths.
  std::recursive_mutex _filesnodes_mutex;
  // Global map of filename / filenode for all the filesystem.
  std::unordered_map<std::wstring, std::shared_ptr<filenode>> _filenodes;
  // Directory map of directoryname / sub filenodes in the scope.
  // A directory \foo with 2 files bar and coco will have one entry:
  // first: foo - second: set filenode { bar, coco }
  std::unordered_map<std::wstring, std::set<std::shared_ptr<filenode>>> _directoryPaths;
};

class memfs {
public:
  memfs() = default;
  // Start the memory filesystem and block until unmount.
  void start();
  void wait();
  void stop();

  void* instance = nullptr;

  // FileSystem mount options
  WCHAR mount_point[MAX_PATH] = L"M:\\";
  WCHAR unc_name[MAX_PATH] = L"";
  bool single_thread = false;
  bool network_drive = false;
  bool removable_drive = false;
  bool current_session = false;
  bool debug_log = false;
  bool enable_network_unmount = false;
  bool dispatch_driver_logs = false;
  uint32_t timeout = 0;

  // Memory FileSystem runtime context.
  std::unique_ptr<fs_filenodes> fs_filenodes;
};

// Memfs Dokan API implementation.
extern iris_vfs_operations memfs_operations;
// Helper getting the memfs filenodes context at each Dokan API call.
#define GET_FS_INSTANCE                                                                            \
  reinterpret_cast<memfs*>(dokanfileinfo->options->global_context)->fs_filenodes.get()
} // namespace memfs

class MyFs final : public iris::vfs {
public:
  MyFs()
    : iris::vfs(L"P:\\",
                L"",
                (iris_vfs_option)(iris_vfs_option_alt_stream | iris_vfs_option_current_session
                                  | iris_vfs_option_debug | iris_vfs_option_stderr
                                  | iris_vfs_option_dispatch_driver_logs)) {}
};
/*
bcdedit /set nointegritychecks on
bcdedit /set loadoptions DISABLE_INTEGRITY_CHECKS
bcdedit /set testsigning on
*/

#if 0
int main(int argc, char* argv[]) {
  iris_vfs_version ver = {};
  int ret = iris_vfs_get_version(&ver);
  auto fs = std::make_unique<MyFs>();
  fs->wait_for_closed();
  return 0;
}
#endif

#include "spdlog/spdlog.h"

std::shared_ptr<memfs::memfs> dokan_memfs;

BOOL WINAPI ctrl_handler(DWORD dw_ctrl_type) {
  switch (dw_ctrl_type) {
  case CTRL_C_EVENT:
  case CTRL_BREAK_EVENT:
  case CTRL_CLOSE_EVENT:
  case CTRL_LOGOFF_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    SetConsoleCtrlHandler(ctrl_handler, FALSE);
    dokan_memfs->stop();
    return TRUE;
  default:
    return FALSE;
  }
}

// /c /n \kfs\myfs
int __cdecl wmain(ULONG argc, PWCHAR argv[]) {
  try {
    dokan_memfs = std::make_shared<memfs::memfs>();
    // Parse arguments
    for (ULONG i = 1; i < argc; ++i) {
      std::wstring arg = argv[i];
      if (arg == L"/h") {
        //show_usage();
        return 0;
      } else if (arg == L"/m") {
        dokan_memfs->removable_drive = true;
      } else if (arg == L"/c") {
        dokan_memfs->current_session = true;
      } else if (arg == L"/d") {
        dokan_memfs->debug_log = true;
      } else if (arg == L"/x") {
        dokan_memfs->enable_network_unmount = true;
      } else if (arg == L"/e") {
        dokan_memfs->dispatch_driver_logs = true;
      } else if (arg == L"/t") {
        dokan_memfs->single_thread = true;
      } else {
        if (i + 1 >= argc) {
          //show_usage();
          return 1;
        }
        std::wstring extra_arg = argv[++i];
        if (arg == L"/i") {
          dokan_memfs->timeout = std::stoul(extra_arg);
        } else if (arg == L"/l") {
          wcscpy_s(dokan_memfs->mount_point, sizeof(dokan_memfs->mount_point) / sizeof(WCHAR),
                   extra_arg.c_str());
        } else if (arg == L"/n") {
          dokan_memfs->network_drive = true;
          wcscpy_s(dokan_memfs->unc_name, sizeof(dokan_memfs->unc_name) / sizeof(WCHAR),
                   extra_arg.c_str());
        }
      }
    }
    if (!SetConsoleCtrlHandler(ctrl_handler, TRUE)) {
      spdlog::error("Control Handler is not set: {}", GetLastError());
    }
    // Start the memory filesystem
    dokan_memfs->start();
    dokan_memfs->wait();
  } catch (const std::exception& ex) {
    spdlog::error("dokan_memfs failure: {}", ex.what());
    return 1;
  }
  return 0;
}

#include <iostream>
#include <mutex>
#include <sddl.h>
#include <sstream>
#include <unordered_map>

/** Dokan mount succeed. */
#define DOKAN_SUCCESS 0
/** Dokan mount error. */
#define DOKAN_ERROR -1
/** Dokan mount failed - Bad drive letter. */
#define DOKAN_DRIVE_LETTER_ERROR -2
/** Dokan mount failed - Can't install driver.  */
#define DOKAN_DRIVER_INSTALL_ERROR -3
/** Dokan mount failed - Driver answer that something is wrong. */
#define DOKAN_START_ERROR -4
/**
 * Dokan mount failed.
 * Can't assign a drive letter or mount point.
 * Probably already used by another volume.
 */
#define DOKAN_MOUNT_ERROR -5
/**
 * Dokan mount failed.
 * Mount point is invalid.
 */
#define DOKAN_MOUNT_POINT_ERROR -6
/**
 * Dokan mount failed.
 * Requested an incompatible version.
 */
#define DOKAN_VERSION_ERROR -7

#define FILE_SUPERSEDE 0x00000000
#define FILE_OPEN 0x00000001
#define FILE_CREATE 0x00000002
#define FILE_OPEN_IF 0x00000003
#define FILE_OVERWRITE 0x00000004
#define FILE_OVERWRITE_IF 0x00000005
#define FILE_MAXIMUM_DISPOSITION 0x00000005

#define FILE_DIRECTORY_FILE 0x00000001
#define FILE_WRITE_THROUGH 0x00000002
#define FILE_SEQUENTIAL_ONLY 0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING 0x00000008

#define FILE_SYNCHRONOUS_IO_ALERT 0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT 0x00000020
#define FILE_NON_DIRECTORY_FILE 0x00000040
#define FILE_CREATE_TREE_CONNECTION 0x00000080

#define FILE_COMPLETE_IF_OPLOCKED 0x00000100
#define FILE_NO_EA_KNOWLEDGE 0x00000200
#define FILE_OPEN_REMOTE_INSTANCE 0x00000400
#define FILE_RANDOM_ACCESS 0x00000800

#define FILE_DELETE_ON_CLOSE 0x00001000
#define FILE_OPEN_BY_FILE_ID 0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT 0x00004000
#define FILE_NO_COMPRESSION 0x00008000

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
#define FILE_OPEN_REQUIRING_OPLOCK 0x00010000
#define FILE_DISALLOW_EXCLUSIVE 0x00020000
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN7 */
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
#define FILE_SESSION_AWARE 0x00040000
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN7 */

#define FILE_RESERVE_OPFILTER 0x00100000
#define FILE_OPEN_REPARSE_POINT 0x00200000
#define FILE_OPEN_NO_RECALL 0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY 0x00800000

#define FILE_VALID_OPTION_FLAGS 0x00ffffff

#define FILE_SUPERSEDED 0x00000000
#define FILE_OPENED 0x00000001
#define FILE_CREATED 0x00000002
#define FILE_OVERWRITTEN 0x00000003
#define FILE_EXISTS 0x00000004
#define FILE_DOES_NOT_EXIST 0x00000005

#define FILE_WRITE_TO_END_OF_FILE 0xffffffff
#define FILE_USE_FILE_POINTER_POSITION 0xfffffffe

namespace memfs {
void memfs::start() {
  fs_filenodes = std::make_unique<::memfs::fs_filenodes>();

  iris_vfs_options dokan_options;
  ZeroMemory(&dokan_options, sizeof(iris_vfs_options));
  dokan_options.version = 200;
  dokan_options.options = iris_vfs_option_alt_stream | iris_vfs_option_case_sensitive;
  dokan_options.mount_point = mount_point;
  dokan_options.single_thread = single_thread;
  if (debug_log) {
    dokan_options.options |= iris_vfs_option_stderr | iris_vfs_option_debug;
    if (dispatch_driver_logs) {
      dokan_options.options |= iris_vfs_option_dispatch_driver_logs;
    }
  } else {
    spdlog::set_level(spdlog::level::err);
  }
  // Mount type
  if (network_drive) {
    dokan_options.options |= iris_vfs_option_network;
    dokan_options.unc_name = unc_name;
    if (enable_network_unmount) {
      dokan_options.options |= iris_vfs_option_enable_unmount_netdrive;
    }
  } else if (removable_drive) {
    dokan_options.options |= iris_vfs_option_removable;
  } else {
    dokan_options.options |= iris_vfs_option_mount_manager;
  }

  if (current_session && (dokan_options.options & iris_vfs_option_mount_manager) == 0) {
    dokan_options.options |= iris_vfs_option_current_session;
  }

  dokan_options.timeout = timeout;
  dokan_options.global_context = reinterpret_cast<ULONG64>(this);

  int status = iris::vfs_api_instance::g().create(&dokan_options, &memfs_operations, &instance);
  switch (status) {
  case DOKAN_SUCCESS:
    break;
  case DOKAN_ERROR:
    throw std::runtime_error("Error");
  case DOKAN_DRIVE_LETTER_ERROR:
    throw std::runtime_error("Bad Drive letter");
  case DOKAN_DRIVER_INSTALL_ERROR:
    throw std::runtime_error("Can't install driver");
  case DOKAN_START_ERROR:
    throw std::runtime_error("Driver something wrong");
  case DOKAN_MOUNT_ERROR:
    throw std::runtime_error("Can't assign a drive letter");
  case DOKAN_MOUNT_POINT_ERROR:
    throw std::runtime_error("Mount point error");
  case DOKAN_VERSION_ERROR:
    throw std::runtime_error("Version error");
  default:
    spdlog::error(L"DokanMain failed with {}", status);
    throw std::runtime_error("Unknown error"); // add error status
  }
}

void memfs::wait() {
  iris::vfs_api_instance::g().wait(instance);
}

void memfs::stop() {
  iris::vfs_api_instance::g().destroy(instance);
}

const std::wstring memfs_helper::DataStreamNameStr = std::wstring(L":$DATA");
static const uint32_t g_volumserial = 0x19833116;

static int create_main_stream(fs_filenodes* fs_filenodes,
                              const std::wstring& filename,
                              const std::pair<std::wstring, std::wstring>& stream_names,
                              uint32_t file_attributes_and_flags,
                              /*PDOKAN_IO_SECURITY_CONTEXT*/ void* security_context) {
  // When creating a new a alternated stream, we need to be sure
  // the main stream exist otherwise we create it.
  auto main_stream_name = memfs_helper::GetFileNameStreamLess(filename, stream_names);
  if (!fs_filenodes->find(main_stream_name)) {
    spdlog::info(L"create_main_stream: we create the maing stream {}", main_stream_name);
    auto n
        = fs_filenodes->add(std::make_shared<filenode>(main_stream_name, false,
                                                       file_attributes_and_flags, security_context),
                            {});
    if (n != STATUS_SUCCESS)
      return n;
  }
  return STATUS_SUCCESS;
}

static int __stdcall memfs_createfile(const wchar_t* filename,
                                      /*PDOKAN_IO_SECURITY_CONTEXT*/ void* security_context,
                                      uint32_t desiredaccess,
                                      uint32_t fileattributes,
                                      uint32_t /*shareaccess*/,
                                      uint32_t createdisposition,
                                      uint32_t createoptions,
                                      iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  uint32_t generic_desiredaccess;
  uint32_t creation_disposition;
  uint32_t file_attributes_and_flags;

  iris_vfs_convert_flags(desiredaccess, fileattributes, createoptions, createdisposition,
                         &generic_desiredaccess, (uint32_t*)&file_attributes_and_flags,
                         (uint32_t*)&creation_disposition);

  auto filename_str = std::wstring(filename);
  memfs_helper::RemoveStreamType(filename_str);

  auto f = filenodes->find(filename_str);
  auto stream_names = memfs_helper::GetStreamNames(filename_str);

  spdlog::info(L"CreateFile: {} with node: {}", filename_str, (f != nullptr));

  // We only support filename length under 255.
  // See GetVolumeInformation - MaximumComponentLength
  if (stream_names.first.length() > 255)
    return STATUS_OBJECT_NAME_INVALID;

  // Windows will automatically try to create and access different system
  // directories.
  if (filename_str == L"\\System Volume Information" || filename_str == L"\\$RECYCLE.BIN") {
    return STATUS_NO_SUCH_FILE;
  }

  if (f && f->is_directory) {
    if (createoptions & FILE_NON_DIRECTORY_FILE)
      return STATUS_FILE_IS_A_DIRECTORY;
    dokanfileinfo->is_dir = true;
  }

  // TODO Use AccessCheck to check security rights

  if (dokanfileinfo->is_dir) {
    spdlog::info(L"CreateFile: {} is a Directory", filename_str);

    if (creation_disposition == CREATE_NEW || creation_disposition == OPEN_ALWAYS) {
      spdlog::info(L"CreateFile: {} create Directory", filename_str);
      // Cannot create a stream as directory.
      if (!stream_names.second.empty())
        return STATUS_NOT_A_DIRECTORY;

      if (f)
        return STATUS_OBJECT_NAME_COLLISION;

      auto newfileNode = std::make_shared<filenode>(filename_str, true, FILE_ATTRIBUTE_DIRECTORY,
                                                    security_context);
      return filenodes->add(newfileNode, stream_names);
    }

    if (f && !f->is_directory)
      return STATUS_NOT_A_DIRECTORY;
    if (!f)
      return STATUS_OBJECT_NAME_NOT_FOUND;

    spdlog::info(L"CreateFile: {} open Directory", filename_str);
  } else {
    spdlog::info(L"CreateFile: {} is a File", filename_str);

    // Cannot overwrite an hidden or system file.
    if (f
        && (((!(file_attributes_and_flags & FILE_ATTRIBUTE_HIDDEN)
              && (f->attributes & FILE_ATTRIBUTE_HIDDEN))
             || (!(file_attributes_and_flags & FILE_ATTRIBUTE_SYSTEM)
                 && (f->attributes & FILE_ATTRIBUTE_SYSTEM)))
            && (creation_disposition == TRUNCATE_EXISTING
                || creation_disposition == CREATE_ALWAYS)))
      return STATUS_ACCESS_DENIED;

    // Cannot delete a file with readonly attributes.
    if ((f && (f->attributes & FILE_ATTRIBUTE_READONLY)
         || (file_attributes_and_flags & FILE_ATTRIBUTE_READONLY))
        && (file_attributes_and_flags & FILE_FLAG_DELETE_ON_CLOSE))
      return STATUS_CANNOT_DELETE;

    // Cannot open a readonly file for writing.
    if ((creation_disposition == OPEN_ALWAYS || creation_disposition == OPEN_EXISTING) && f
        && (f->attributes & FILE_ATTRIBUTE_READONLY) && desiredaccess & FILE_WRITE_DATA)
      return STATUS_ACCESS_DENIED;

    // Cannot overwrite an existing read only file.
    // FILE_SUPERSEDE can as it replace and not overwrite.
    if ((creation_disposition == CREATE_NEW
         || (creation_disposition == CREATE_ALWAYS && createdisposition != FILE_SUPERSEDE)
         || creation_disposition == TRUNCATE_EXISTING)
        && f && (f->attributes & FILE_ATTRIBUTE_READONLY))
      return STATUS_ACCESS_DENIED;

    if (creation_disposition == CREATE_NEW || creation_disposition == CREATE_ALWAYS
        || creation_disposition == OPEN_ALWAYS || creation_disposition == TRUNCATE_EXISTING) {
      // Combines the file attributes and flags specified by
      // dwFlagsAndAttributes with FILE_ATTRIBUTE_ARCHIVE.
      file_attributes_and_flags |= FILE_ATTRIBUTE_ARCHIVE;
      // We merge the attributes with the existing file attributes
      // except for FILE_SUPERSEDE.
      if (f && createdisposition != FILE_SUPERSEDE)
        file_attributes_and_flags |= f->attributes;
      // Remove non specific attributes.
      file_attributes_and_flags &= ~FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL;
      // FILE_ATTRIBUTE_NORMAL is override if any other attribute is set.
      file_attributes_and_flags &= ~FILE_ATTRIBUTE_NORMAL;
    }

    switch (creation_disposition) {
    case CREATE_ALWAYS: {
      spdlog::info(L"CreateFile: {} CREATE_ALWAYS", filename_str);
      /*
       * Creates a new file, always.
       *
       * We handle FILE_SUPERSEDE here as it is converted to TRUNCATE_EXISTING
       * by DokanMapKernelToUserCreateFileFlags.
       */

      if (!stream_names.second.empty()) {
        // The createfile is a alternate stream,
        // we need to be sure main stream exist
        auto n = create_main_stream(filenodes, filename_str, stream_names,
                                    file_attributes_and_flags, security_context);
        if (n != STATUS_SUCCESS)
          return n;
      }

      auto n
          = filenodes->add(std::make_shared<filenode>(filename_str, false,
                                                      file_attributes_and_flags, security_context),
                           stream_names);
      if (n != STATUS_SUCCESS)
        return n;

      /*
       * If the specified file exists and is writable, the function overwrites
       * the file, the function succeeds, and last-error code is set to
       * ERROR_ALREADY_EXISTS
       */
      if (f)
        return STATUS_OBJECT_NAME_COLLISION;
    } break;
    case CREATE_NEW: {
      spdlog::info(L"CreateFile: {} CREATE_NEW", filename_str);
      /*
       * Creates a new file, only if it does not already exist.
       */
      if (f)
        return STATUS_OBJECT_NAME_COLLISION;

      if (!stream_names.second.empty()) {
        // The createfile is a alternate stream,
        // we need to be sure main stream exist
        auto n = create_main_stream(filenodes, filename_str, stream_names,
                                    file_attributes_and_flags, security_context);
        if (n != STATUS_SUCCESS)
          return n;
      }

      auto n
          = filenodes->add(std::make_shared<filenode>(filename_str, false,
                                                      file_attributes_and_flags, security_context),
                           stream_names);
      if (n != STATUS_SUCCESS)
        return n;
    } break;
    case OPEN_ALWAYS: {
      spdlog::info(L"CreateFile: {} OPEN_ALWAYS", filename_str);
      /*
       * Opens a file, always.
       */

      if (!f) {
        auto n = filenodes->add(std::make_shared<filenode>(filename_str, false,
                                                           file_attributes_and_flags,
                                                           security_context),
                                stream_names);
        if (n != STATUS_SUCCESS)
          return n;
      } else {
        if (desiredaccess & FILE_EXECUTE) {
          f->times.lastaccess = filetimes::get_currenttime();
        }
      }
    } break;
    case OPEN_EXISTING: {
      spdlog::info(L"CreateFile: {} OPEN_EXISTING", filename_str);
      /*
       * Opens a file or device, only if it exists.
       * If the specified file or device does not exist, the function fails
       * and the last-error code is set to ERROR_FILE_NOT_FOUND
       */
      if (!f)
        return STATUS_OBJECT_NAME_NOT_FOUND;

      if (desiredaccess & FILE_EXECUTE) {
        f->times.lastaccess = filetimes::get_currenttime();
      }
    } break;
    case TRUNCATE_EXISTING: {
      spdlog::info(L"CreateFile: {} TRUNCATE_EXISTING", filename_str);
      /*
       * Opens a file and truncates it so that its size is zero bytes, only if
       * it exists. If the specified file does not exist, the function fails
       * and the last-error code is set to ERROR_FILE_NOT_FOUND
       */
      if (!f)
        return STATUS_OBJECT_NAME_NOT_FOUND;

      f->set_endoffile(0);
      f->times.lastaccess = f->times.lastwrite = filetimes::get_currenttime();
      f->attributes = file_attributes_and_flags;
    } break;
    default:
      spdlog::info(L"CreateFile: {} Unknown CreationDisposition {}", filename_str,
                   creation_disposition);
      break;
    }
  }

  /*
   * CREATE_NEW && OPEN_ALWAYS
   * If the specified file exists, the function fails and the last-error code is
   * set to ERROR_FILE_EXISTS
   */
  if (f && (creation_disposition == CREATE_NEW || creation_disposition == OPEN_ALWAYS))
    return STATUS_OBJECT_NAME_COLLISION;

  return STATUS_SUCCESS;
}

static void __stdcall memfs_cleanup(const wchar_t* filename, iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  spdlog::info(L"Cleanup: {}", filename_str);
  if (dokanfileinfo->delete_on_close) {
    // Delete happens during cleanup and not in close event.
    spdlog::info(L"\tDeleteOnClose: {}", filename_str);
    filenodes->remove(filename_str);
  }
}

static void __stdcall memfs_closeFile(const wchar_t* filename,
                                      iris_vfs_file_info_ptr /*dokanfileinfo*/) {
  auto filename_str = std::wstring(filename);
  // Here we should release all resources from the createfile context if we had.
  spdlog::info(L"CloseFile: {}", filename_str);
}

static int __stdcall memfs_readfile(const wchar_t* filename,
                                    LPVOID buffer,
                                    uint32_t bufferlength,
                                    uint32_t* readlength,
                                    LONGLONG offset,
                                    iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  spdlog::info(L"ReadFile: {}", filename_str);
  auto f = filenodes->find(filename_str);
  if (!f)
    return STATUS_OBJECT_NAME_NOT_FOUND;

  *readlength = f->read(buffer, bufferlength, offset);
  spdlog::info(L"\tBufferLength: {} offset: {} readlength: {}", bufferlength, offset, *readlength);
  return STATUS_SUCCESS;
}

static int __stdcall memfs_writefile(const wchar_t* filename,
                                     LPCVOID buffer,
                                     uint32_t number_of_bytes_to_write,
                                     uint32_t* number_of_bytes_written,
                                     LONGLONG offset,
                                     iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  spdlog::info(L"WriteFile: {}", filename_str);
  auto f = filenodes->find(filename_str);
  if (!f)
    return STATUS_OBJECT_NAME_NOT_FOUND;

  auto file_size = f->get_filesize();

  // An Offset -1 is like the file was opened with FILE_APPEND_DATA
  // and we need to write at the end of the file.
  if (offset == -1)
    offset = file_size;

  if (dokanfileinfo->paging_io) {
    // PagingIo cannot extend file size.
    // We return STATUS_SUCCESS when offset is beyond fileSize
    // and write the maximum we are allowed to.
    if (offset >= file_size) {
      spdlog::info(L"\tPagingIo Outside offset: {} FileSize: {}", offset, file_size);
      *number_of_bytes_written = 0;
      return STATUS_SUCCESS;
    }

    if ((offset + number_of_bytes_to_write) > file_size) {
      // resize the write length to not go beyond file size.
      LONGLONG bytes = file_size - offset;
      if (bytes >> 32) {
        number_of_bytes_to_write = static_cast<uint32_t>(bytes & 0xFFFFFFFFUL);
      } else {
        number_of_bytes_to_write = static_cast<uint32_t>(bytes);
      }
    }
    spdlog::info(L"\tPagingIo number_of_bytes_to_write: {}", number_of_bytes_to_write);
  }

  *number_of_bytes_written = f->write(buffer, number_of_bytes_to_write, offset);

  spdlog::info(L"\tNumberOfBytesToWrite {} offset: {} number_of_bytes_written: {}",
               number_of_bytes_to_write, offset, *number_of_bytes_written);
  return STATUS_SUCCESS;
}

static int __stdcall memfs_flushfilebuffers(const wchar_t* filename,
                                            iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  spdlog::info(L"FlushFileBuffers: {}", filename_str);
  auto f = filenodes->find(filename_str);
  // Nothing to flush, we directly write the content into our buffer.

  if (f->main_stream)
    f = f->main_stream;
  f->times.lastaccess = f->times.lastwrite = filetimes::get_currenttime();

  return STATUS_SUCCESS;
}

static int __stdcall memfs_getfileInformation(const wchar_t* filename,
                                              by_handle_file_info_ptr rbuffer,
                                              iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  spdlog::info(L"GetFileInformation: {}", filename_str);
  auto f = filenodes->find(filename_str);
  if (!f)
    return STATUS_OBJECT_NAME_NOT_FOUND;
  LPBY_HANDLE_FILE_INFORMATION buffer = (LPBY_HANDLE_FILE_INFORMATION)rbuffer;
  buffer->dwFileAttributes = f->attributes;
  memfs_helper::LlongToFileTime(f->times.creation, buffer->ftCreationTime);
  memfs_helper::LlongToFileTime(f->times.lastaccess, buffer->ftLastAccessTime);
  memfs_helper::LlongToFileTime(f->times.lastwrite, buffer->ftLastWriteTime);
  auto strLength = f->get_filesize();
  memfs_helper::LlongToDwLowHigh(strLength, (uint32_t&)buffer->nFileSizeLow,
                                 (uint32_t&)buffer->nFileSizeHigh);
  memfs_helper::LlongToDwLowHigh(f->fileindex, (uint32_t&)buffer->nFileIndexLow,
                                 (uint32_t&)buffer->nFileIndexHigh);
  // We do not track the number of links to the file so we return a fake value.
  buffer->nNumberOfLinks = 1;
  buffer->dwVolumeSerialNumber = g_volumserial;

  spdlog::info(
      L"GetFileInformation: {} Attributes: {:x} Times: Creation {:x} "
      L"LastAccess {:x} LastWrite {:x} FileSize {} NumberOfLinks {} "
      L"VolumeSerialNumber {:x}",
      filename_str, f->attributes, f->times.creation, f->times.lastaccess, f->times.lastwrite,
      strLength, buffer->nNumberOfLinks, buffer->dwVolumeSerialNumber);

  return STATUS_SUCCESS;
}

static int __stdcall memfs_findfiles(const wchar_t* filename,
                                     fn_fill_find_dataw fill_finddata,
                                     iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  auto files = filenodes->list_folder(filename_str);
  WIN32_FIND_DATAW findData;
  spdlog::info(L"FindFiles: {}", filename_str);
  ZeroMemory(&findData, sizeof(WIN32_FIND_DATAW));
  for (const auto& f : files) {
    if (f->main_stream)
      continue; // Do not list File Streams
    const auto fileNodeName = memfs_helper::GetFileName(f->get_filename());
    if (fileNodeName.size() > MAX_PATH)
      continue;
    std::copy(fileNodeName.begin(), fileNodeName.end(), std::begin(findData.cFileName));
    findData.cFileName[fileNodeName.length()] = '\0';
    findData.dwFileAttributes = f->attributes;
    memfs_helper::LlongToFileTime(f->times.creation, findData.ftCreationTime);
    memfs_helper::LlongToFileTime(f->times.lastaccess, findData.ftLastAccessTime);
    memfs_helper::LlongToFileTime(f->times.lastwrite, findData.ftLastWriteTime);
    auto file_size = f->get_filesize();
    memfs_helper::LlongToDwLowHigh(file_size, (uint32_t&)findData.nFileSizeLow,
                                   (uint32_t&)findData.nFileSizeHigh);
    spdlog::info(
        L"FindFiles: {} fileNode: {} Attributes: {} Times: Creation {} "
        L"LastAccess {} LastWrite {} FileSize {}",
        filename_str, fileNodeName, findData.dwFileAttributes, f->times.creation,
        f->times.lastaccess, f->times.lastwrite, file_size);
    fill_finddata((find_dataw_ptr)&findData, dokanfileinfo);
  }
  return STATUS_SUCCESS;
}

static int __stdcall memfs_setfileattributes(const wchar_t* filename,
                                             uint32_t fileattributes,
                                             iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  auto f = filenodes->find(filename_str);
  spdlog::info(L"SetFileAttributes: {} fileattributes {}", filename_str, fileattributes);
  if (!f)
    return STATUS_OBJECT_NAME_NOT_FOUND;

  // No attributes need to be changed
  if (fileattributes == 0)
    return STATUS_SUCCESS;

  // FILE_ATTRIBUTE_NORMAL is override if any other attribute is set
  if (fileattributes & FILE_ATTRIBUTE_NORMAL && (fileattributes & (fileattributes - 1)))
    fileattributes &= ~FILE_ATTRIBUTE_NORMAL;

  f->attributes = fileattributes;
  return STATUS_SUCCESS;
}

static int __stdcall memfs_setfiletime(const wchar_t* filename,
                                       CONST file_time* creationtime,
                                       CONST file_time* lastaccesstime,
                                       CONST file_time* lastwritetime,
                                       iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  auto f = filenodes->find(filename_str);
  spdlog::info(L"SetFileTime: {}", filename_str);
  if (!f)
    return STATUS_OBJECT_NAME_NOT_FOUND;
  if (creationtime && !filetimes::empty((const FILETIME*)creationtime))
    f->times.creation = memfs_helper::FileTimeToLlong((const FILETIME&)*creationtime);
  if (lastaccesstime && !filetimes::empty((const FILETIME*)lastaccesstime))
    f->times.lastaccess = memfs_helper::FileTimeToLlong((const FILETIME&)*lastaccesstime);
  if (lastwritetime && !filetimes::empty((const FILETIME*)lastwritetime))
    f->times.lastwrite = memfs_helper::FileTimeToLlong((const FILETIME&)*lastwritetime);
  // We should update Change Time here but dokan use lastwritetime for both.
  return STATUS_SUCCESS;
}

static int __stdcall memfs_deletefile(const wchar_t* filename,
                                      iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  auto f = filenodes->find(filename_str);
  spdlog::info(L"DeleteFile: {}", filename_str);

  if (!f)
    return STATUS_OBJECT_NAME_NOT_FOUND;

  if (f->is_directory)
    return STATUS_ACCESS_DENIED;

  // Here prepare and check if the file can be deleted
  // or if delete is canceled when dokanfileinfo->DeleteOnClose false

  return STATUS_SUCCESS;
}

static int __stdcall memfs_deletedirectory(const wchar_t* filename,
                                           iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  spdlog::info(L"DeleteDirectory: {}", filename_str);

  if (filenodes->list_folder(filename_str).size())
    return STATUS_DIRECTORY_NOT_EMPTY;

  // Here prepare and check if the directory can be deleted
  // or if delete is canceled when dokanfileinfo->DeleteOnClose false

  return STATUS_SUCCESS;
}

static int __stdcall memfs_movefile(const wchar_t* filename,
                                    const wchar_t* new_filename,
                                    uint8_t replace_if_existing,
                                    iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  auto new_filename_str = std::wstring(new_filename);
  spdlog::info(L"MoveFile: {} to {}", filename_str, new_filename_str);
  memfs_helper::RemoveStreamType(new_filename_str);
  auto new_stream_names = memfs_helper::GetStreamNames(new_filename_str);
  if (new_stream_names.first.empty()) {
    // new_filename is a stream name :<stream name>:<stream type>
    // We removed the stream type and now need to concat the filename and the
    // new stream name
    auto stream_names = memfs_helper::GetStreamNames(filename_str);
    new_filename_str = memfs_helper::GetFileNameStreamLess(filename, stream_names) + L":"
                       + new_stream_names.second;
  }
  spdlog::info(L"MoveFile: after {} to {}", filename_str, new_filename_str);
  return filenodes->move(filename_str, new_filename_str, replace_if_existing);
}

static int __stdcall memfs_setendoffile(const wchar_t* filename,
                                        LONGLONG ByteOffset,
                                        iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  spdlog::info(L"SetEndOfFile: {} ByteOffset {}", filename_str, ByteOffset);
  auto f = filenodes->find(filename_str);

  if (!f)
    return STATUS_OBJECT_NAME_NOT_FOUND;
  f->set_endoffile(ByteOffset);
  return STATUS_SUCCESS;
}

static int __stdcall memfs_setallocationsize(const wchar_t* filename,
                                             LONGLONG alloc_size,
                                             iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  spdlog::info(L"SetAllocationSize: {} AllocSize {}", filename_str, alloc_size);
  auto f = filenodes->find(filename_str);

  if (!f)
    return STATUS_OBJECT_NAME_NOT_FOUND;
  f->set_endoffile(alloc_size);
  return STATUS_SUCCESS;
}

static int __stdcall memfs_lockfile(const wchar_t* filename,
                                    LONGLONG byte_offset,
                                    LONGLONG length,
                                    iris_vfs_file_info_ptr dokanfileinfo) {
  auto filename_str = std::wstring(filename);
  spdlog::info(L"LockFile: {} ByteOffset {} Length {}", filename_str, byte_offset, length);
  return STATUS_NOT_IMPLEMENTED;
}

static int __stdcall memfs_unlockfile(const wchar_t* filename,
                                      LONGLONG byte_offset,
                                      LONGLONG length,
                                      iris_vfs_file_info_ptr dokanfileinfo) {
  auto filename_str = std::wstring(filename);
  spdlog::info(L"UnlockFile: {} ByteOffset {} Length {}", filename_str, byte_offset, length);
  return STATUS_NOT_IMPLEMENTED;
}

static int __stdcall memfs_getdiskfreespace(PULONGLONG free_bytes_available,
                                            PULONGLONG total_number_of_bytes,
                                            PULONGLONG total_number_of_free_bytes,
                                            iris_vfs_file_info_ptr dokanfileinfo) {
  spdlog::info(L"GetDiskFreeSpace");
  *free_bytes_available = (ULONGLONG)(512 * 1024 * 1024);
  *total_number_of_bytes = MAXLONGLONG;
  *total_number_of_free_bytes = MAXLONGLONG;
  return STATUS_SUCCESS;
}

static int __stdcall memfs_getvolumeinformation(LPWSTR volumename_buffer,
                                                uint32_t volumename_size,
                                                uint32_t* volume_serialnumber,
                                                uint32_t* maximum_component_length,
                                                uint32_t* filesystem_flags,
                                                LPWSTR filesystem_name_buffer,
                                                uint32_t filesystem_name_size,
                                                iris_vfs_file_info_ptr /*dokanfileinfo*/) {
  spdlog::info(L"GetVolumeInformation");
  wcscpy_s(volumename_buffer, volumename_size, L"Iris MemFS");
  *volume_serialnumber = g_volumserial;
  *maximum_component_length = 255;
  *filesystem_flags = FILE_CASE_SENSITIVE_SEARCH | FILE_CASE_PRESERVED_NAMES
                      | FILE_SUPPORTS_REMOTE_STORAGE | FILE_UNICODE_ON_DISK | FILE_NAMED_STREAMS;

  wcscpy_s(filesystem_name_buffer, filesystem_name_size, L"NTFS");
  return STATUS_SUCCESS;
}

static int __stdcall memfs_mounted(const wchar_t* MountPoint,
                                   iris_vfs_file_info_ptr dokanfileinfo) {
  spdlog::info(L"Mounted as {}", MountPoint);
  WCHAR* mount_point
      = (reinterpret_cast<memfs*>(dokanfileinfo->options->global_context))->mount_point;
  wcscpy_s(mount_point, sizeof(WCHAR) * MAX_PATH, MountPoint);
  return STATUS_SUCCESS;
}

static int __stdcall memfs_unmounted(iris_vfs_file_info_ptr /*dokanfileinfo*/) {
  spdlog::info(L"Unmounted");
  return STATUS_SUCCESS;
}

static int __stdcall memfs_getfilesecurity(const wchar_t* filename,
                                           PSECURITY_INFORMATION security_information,
                                           PSECURITY_DESCRIPTOR security_descriptor,
                                           uint32_t bufferlength,
                                           PULONG length_needed,
                                           iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  spdlog::info(L"GetFileSecurity: {}", filename_str);
  auto f = filenodes->find(filename_str);

  if (!f)
    return STATUS_OBJECT_NAME_NOT_FOUND;

  std::lock_guard<std::mutex> lockFile(f->security);

  // This will make dokan library return a default security descriptor
  if (!f->security.descriptor)
    return STATUS_NOT_IMPLEMENTED;

  // We have a Security Descriptor but we need to extract only informations
  // requested 1 - Convert the Security Descriptor to SDDL string with the
  // informations requested
  LPTSTR pStringBuffer = NULL;
  if (!ConvertSecurityDescriptorToStringSecurityDescriptor(f->security.descriptor.get(),
                                                           SDDL_REVISION_1, *security_information,
                                                           &pStringBuffer, NULL)) {
    return STATUS_NOT_IMPLEMENTED;
  }

  // 2 - Convert the SDDL string back to Security Descriptor
  PSECURITY_DESCRIPTOR SecurityDescriptorTmp = NULL;
  uint32_t Size = 0;
  if (!ConvertStringSecurityDescriptorToSecurityDescriptor(pStringBuffer, SDDL_REVISION_1,
                                                            &SecurityDescriptorTmp,
                                                            (PULONG)&Size)) {
    LocalFree(pStringBuffer);
    return STATUS_NOT_IMPLEMENTED;
  }
  LocalFree(pStringBuffer);

  *length_needed = Size;
  if (Size > bufferlength) {
    LocalFree(SecurityDescriptorTmp);
    return STATUS_BUFFER_OVERFLOW;
  }

  // 3 - Copy the new SecurityDescriptor to destination
  memcpy(security_descriptor, SecurityDescriptorTmp, Size);
  LocalFree(SecurityDescriptorTmp);

  return STATUS_SUCCESS;
}

static int __stdcall memfs_setfilesecurity(const wchar_t* filename,
                                           PSECURITY_INFORMATION security_information,
                                           PSECURITY_DESCRIPTOR security_descriptor,
                                           uint32_t /*bufferlength*/,
                                           iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  spdlog::info(L"SetFileSecurity: {}", filename_str);
  static GENERIC_MAPPING memfs_mapping
      = {FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, FILE_ALL_ACCESS};
  auto f = filenodes->find(filename_str);

  if (!f)
    return STATUS_OBJECT_NAME_NOT_FOUND;

  std::lock_guard<std::mutex> securityLock(f->security);

  // SetPrivateObjectSecurity - ObjectsSecurityDescriptor
  // The memory for the security descriptor must be allocated from the process
  // heap (GetProcessHeap) with the HeapAlloc function.
  // https://devblogs.microsoft.com/oldnewthing/20170727-00/?p=96705
  HANDLE pHeap = GetProcessHeap();
  PSECURITY_DESCRIPTOR heapSecurityDescriptor = HeapAlloc(pHeap, 0, f->security.descriptor_size);
  if (!heapSecurityDescriptor)
    return STATUS_INSUFFICIENT_RESOURCES;
  // Copy our current descriptor into heap memory
  memcpy(heapSecurityDescriptor, f->security.descriptor.get(), f->security.descriptor_size);

  if (!SetPrivateObjectSecurity(*security_information, security_descriptor, &heapSecurityDescriptor,
                                &memfs_mapping, 0)) {
    HeapFree(pHeap, 0, heapSecurityDescriptor);
    return iris_vfs_convert_error(GetLastError());
  }

  f->security.SetDescriptor(heapSecurityDescriptor);
  HeapFree(pHeap, 0, heapSecurityDescriptor);

  return STATUS_SUCCESS;
}

static int __stdcall memfs_findstreams(const wchar_t* filename,
                                       fn_fill_find_stream_data fill_findstreamdata,
                                       PVOID findstreamcontext,
                                       iris_vfs_file_info_ptr dokanfileinfo) {
  auto filenodes = GET_FS_INSTANCE;
  auto filename_str = std::wstring(filename);
  spdlog::info(L"FindStreams: {}", filename_str);
  auto f = filenodes->find(filename_str);

  if (!f)
    return STATUS_OBJECT_NAME_NOT_FOUND;

  auto streams = f->get_streams();

  WIN32_FIND_STREAM_DATA stream_data;
  ZeroMemory(&stream_data, sizeof(WIN32_FIND_STREAM_DATA));

  if (!f->is_directory) {
    // Add the main stream name - \foo::$DATA by returning ::$DATA
    std::copy(memfs_helper::DataStreamNameStr.begin(), memfs_helper::DataStreamNameStr.end(),
              std::begin(stream_data.cStreamName) + 1);
    stream_data.cStreamName[0] = ':';
    stream_data.cStreamName[memfs_helper::DataStreamNameStr.length() + 1] = L'\0';
    stream_data.StreamSize.QuadPart = f->get_filesize();
    if (!fill_findstreamdata((find_stream_data_ptr)&stream_data, findstreamcontext)) {
      return STATUS_BUFFER_OVERFLOW;
    }
  } else if (streams.empty()) {
    // The node is a directory without any alternate streams
    return STATUS_END_OF_FILE;
  }

  // Add the alternated stream attached
  // for \foo:bar we need to return in the form of bar:$DATA
  for (const auto& stream : streams) {
    auto stream_names = memfs_helper::GetStreamNames(stream.first);
    if (stream_names.second.length() + memfs_helper::DataStreamNameStr.length() + 1
        > sizeof(stream_data.cStreamName))
      continue;
    // Copy the filename foo
    std::copy(stream_names.second.begin(), stream_names.second.end(),
              std::begin(stream_data.cStreamName) + 1);
    // Concat :$DATA
    std::copy(memfs_helper::DataStreamNameStr.begin(), memfs_helper::DataStreamNameStr.end(),
              std::begin(stream_data.cStreamName) + stream_names.second.length() + 1);
    stream_data.cStreamName[0] = ':';
    stream_data
        .cStreamName[stream_names.second.length() + memfs_helper::DataStreamNameStr.length() + 1]
        = L'\0';
    stream_data.StreamSize.QuadPart = stream.second->get_filesize();
    spdlog::info(L"FindStreams: {} StreamName: {} Size: {:x}", filename_str, stream_names.second,
                 stream_data.StreamSize.QuadPart);
    if (!fill_findstreamdata((find_stream_data_ptr)&stream_data, findstreamcontext)) {
      return STATUS_BUFFER_OVERFLOW;
    }
  }
  return STATUS_SUCCESS;
}

fs_filenodes::fs_filenodes() {
  WCHAR buffer[4096];
  WCHAR final_buffer[4096];
  PTOKEN_USER user_token = NULL;
  PTOKEN_GROUPS groups_token = NULL;
  HANDLE token_handle;
  LPTSTR user_sid_str = NULL;
  LPTSTR group_sid_str = NULL;

  // Build default root filenode SecurityDescriptor
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token_handle) == FALSE) {
    throw std::runtime_error("Failed init root resources");
  }
  uint32_t return_length;
  if (!GetTokenInformation(token_handle, TokenUser, buffer, sizeof(buffer),
                           (LPDWORD)&return_length)) {
    CloseHandle(token_handle);
    throw std::runtime_error("Failed init root resources");
  }
  user_token = (PTOKEN_USER)buffer;
  if (!ConvertSidToStringSid(user_token->User.Sid, &user_sid_str)) {
    CloseHandle(token_handle);
    throw std::runtime_error("Failed init root resources");
  }
  if (!GetTokenInformation(token_handle, TokenGroups, buffer, sizeof(buffer),
                           (PDWORD)&return_length)) {
    CloseHandle(token_handle);
    throw std::runtime_error("Failed init root resources");
  }
  groups_token = (PTOKEN_GROUPS)buffer;
  if (groups_token->GroupCount > 0) {
    if (!ConvertSidToStringSid(groups_token->Groups[0].Sid, &group_sid_str)) {
      CloseHandle(token_handle);
      throw std::runtime_error("Failed init root resources");
    }
    swprintf_s(buffer, 4096, L"O:%lsG:%ls", user_sid_str, group_sid_str);
  } else {
    swprintf_s(buffer, 4096, L"O:%ls", user_sid_str);
  }
  LocalFree(user_sid_str);
  LocalFree(group_sid_str);
  CloseHandle(token_handle);
  swprintf_s(final_buffer, 4096, L"%lsD:PAI(A;OICI;FA;;;AU)", buffer);
  PSECURITY_DESCRIPTOR security_descriptor = NULL;
  uint32_t size = 0;
  if (!ConvertStringSecurityDescriptorToSecurityDescriptor(final_buffer, SDDL_REVISION_1,
                                                            &security_descriptor, (PULONG)&size))
    throw std::runtime_error("Failed init root resources");
  auto fileNode = std::make_shared<filenode>(L"\\", true, FILE_ATTRIBUTE_DIRECTORY, nullptr);
  fileNode->security.SetDescriptor(security_descriptor);
  LocalFree(security_descriptor);

  _filenodes[L"\\"] = fileNode;
  _directoryPaths.emplace(L"\\", std::set<std::shared_ptr<filenode>>());
}

int fs_filenodes::add(const std::shared_ptr<filenode>& f,
                      std::optional<std::pair<std::wstring, std::wstring>> stream_names) {
  std::lock_guard<std::recursive_mutex> lock(_filesnodes_mutex);

  if (f->fileindex == 0) // previous init
    f->fileindex = _fs_fileindex_count++;
  const auto filename = f->get_filename();
  const auto parent_path = memfs_helper::GetParentPath(filename);

  // Does target folder exist
  if (!_directoryPaths.count(parent_path)) {
    spdlog::warn(L"Add: No directory: {} exist FilePath: {}", parent_path, filename);
    return STATUS_OBJECT_PATH_NOT_FOUND;
  }

  if (!stream_names.has_value())
    stream_names = memfs_helper::GetStreamNames(filename);
  if (!stream_names.value().second.empty()) {
    auto& stream_names_value = stream_names.value();
    spdlog::info(L"Add file: {} is an alternate stream {} and has {} as main stream", filename,
                 stream_names_value.second, stream_names_value.first);
    auto main_stream_name = memfs_helper::GetFileNameStreamLess(filename, stream_names_value);
    auto main_f = find(main_stream_name);
    if (!main_f)
      return STATUS_OBJECT_PATH_NOT_FOUND;
    main_f->add_stream(f);
    f->main_stream = main_f;
    f->fileindex = main_f->fileindex;
  }

  // If we have a folder, we add it to our directoryPaths
  if (f->is_directory && !_directoryPaths.count(filename))
    _directoryPaths.emplace(filename, std::set<std::shared_ptr<filenode>>());

  // Add our file to the fileNodes and directoryPaths
  auto previous_f = _filenodes[filename];
  _filenodes[filename] = f;
  _directoryPaths[parent_path].insert(f);
  if (previous_f)
    _directoryPaths[parent_path].erase(previous_f);

  spdlog::info(L"Add file: {} in folder: {}", filename, parent_path);
  return STATUS_SUCCESS;
}

std::shared_ptr<filenode> fs_filenodes::find(const std::wstring& filename) {
  std::lock_guard<std::recursive_mutex> lock(_filesnodes_mutex);
  auto fileNode = _filenodes.find(filename);
  return (fileNode != _filenodes.end()) ? fileNode->second : nullptr;
}

std::set<std::shared_ptr<filenode>> fs_filenodes::list_folder(const std::wstring& fileName) {
  std::lock_guard<std::recursive_mutex> lock(_filesnodes_mutex);

  auto it = _directoryPaths.find(fileName);
  return (it != _directoryPaths.end()) ? it->second : std::set<std::shared_ptr<filenode>>();
}

void fs_filenodes::remove(const std::wstring& filename) {
  return remove(find(filename));
}

void fs_filenodes::remove(const std::shared_ptr<filenode>& f) {
  if (!f)
    return;

  std::lock_guard<std::recursive_mutex> lock(_filesnodes_mutex);
  auto fileName = f->get_filename();
  spdlog::info(L"Remove: {}", fileName);

  // Remove node from fileNodes and directoryPaths
  _filenodes.erase(fileName);
  _directoryPaths[memfs_helper::GetParentPath(fileName)].erase(f);

  // if it was a directory we need to remove it from directoryPaths
  if (f->is_directory) {
    // but first we need to remove the directory content by looking recursively
    // into it
    auto files = list_folder(fileName);
    for (const auto& file : files)
      remove(file);

    _directoryPaths.erase(fileName);
  }

  // Cleanup streams
  if (f->main_stream) {
    // Is an alternate stream
    f->main_stream->remove_stream(f);
  } else {
    // Is a main stream
    // Remove possible alternate stream
    for (const auto& [stream_name, node] : f->get_streams())
      remove(stream_name);
  }
}

int fs_filenodes::move(const std::wstring& old_filename,
                       const std::wstring& new_filename,
                       uint8_t replace_if_existing) {
  auto f = find(old_filename);
  auto new_f = find(new_filename);

  if (!f)
    return STATUS_OBJECT_NAME_NOT_FOUND;

  // Cannot move to an existing destination without replace flag
  if (!replace_if_existing && new_f)
    return STATUS_OBJECT_NAME_COLLISION;

  // Cannot replace read only destination
  if (new_f && new_f->attributes & FILE_ATTRIBUTE_READONLY)
    return STATUS_ACCESS_DENIED;

  // If destination exist - Cannot move directory or replace a directory
  if (new_f && (f->is_directory || new_f->is_directory))
    return STATUS_ACCESS_DENIED;

  auto newParent_path = memfs_helper::GetParentPath(new_filename);

  std::lock_guard<std::recursive_mutex> lock(_filesnodes_mutex);
  if (!_directoryPaths.count(newParent_path)) {
    spdlog::warn(L"Move: No directory: {} exist FilePath: {}", newParent_path, new_filename);
    return STATUS_OBJECT_PATH_NOT_FOUND;
  }

  // Remove destination
  remove(new_f);

  // Update current node with new data
  const auto fileName = f->get_filename();
  auto oldParentPath = memfs_helper::GetParentPath(fileName);
  f->set_filename(new_filename);

  // Move fileNode
  // 1 - by removing current not with oldName as key
  add(f, {});

  // 2 - If fileNode is a Dir we move content to destination
  if (f->is_directory) {
    // recurse remove sub folders/files
    auto files = list_folder(old_filename);
    for (const auto& file : files) {
      const auto sub_fileName = file->get_filename();
      auto newSubFileName = std::filesystem::path(new_filename)
                                .append(memfs_helper::GetFileName(sub_fileName))
                                .wstring();
      auto n = move(sub_fileName, newSubFileName, replace_if_existing);
      if (n != STATUS_SUCCESS) {
        spdlog::warn(
            L"Move: Subfolder file move {} to {} replaceIfExisting {} failed: "
            L"{}",
            sub_fileName, newSubFileName, replace_if_existing, n);
        return n; // That's bad...we have not done a full move
      }
    }

    // remove folder from directories
    _directoryPaths.erase(old_filename);
  }

  // 3 - Remove fileNode link with oldFilename
  _filenodes.erase(old_filename);
  if (oldParentPath != newParent_path) // Same folder destination
    _directoryPaths[oldParentPath].erase(f);

  spdlog::info(L"Move file: {} to folder: {}", old_filename, new_filename);
  return STATUS_SUCCESS;
}

//---------------------------------------------------------------------------------------------

filenode::filenode(const std::wstring& filename,
                   bool is_directory,
                   uint32_t file_attr,
                   const /*PDOKAN_IO_SECURITY_CONTEXT*/ void* security_context)
  : is_directory(is_directory), attributes(file_attr), _fileName(filename) {
  // No lock need, FileNode is still not in a directory
  times.reset();

  // if (security_context && security_context->AccessState.SecurityDescriptor) {
  //  spdlog::info(L"{} : Attach SecurityDescriptor", filename);
  //  security.SetDescriptor(security_context->AccessState.SecurityDescriptor);
  //}
}

uint32_t filenode::read(LPVOID buffer, uint32_t bufferlength, LONGLONG offset) {
  std::lock_guard<std::mutex> lock(_data_mutex);
  if (static_cast<size_t>(offset + bufferlength) > _data.size())
    bufferlength = (_data.size() > static_cast<size_t>(offset))
                       ? static_cast<uint32_t>(_data.size() - offset)
                       : 0;
  if (bufferlength)
    memcpy(buffer, &_data[static_cast<size_t>(offset)], bufferlength);
  spdlog::info(L"Read {} : BufferLength {} Offset {}", get_filename(), bufferlength, offset);
  return bufferlength;
}

uint32_t filenode::write(LPCVOID buffer, uint32_t number_of_bytes_to_write, LONGLONG offset) {
  if (!number_of_bytes_to_write)
    return 0;

  std::lock_guard<std::mutex> lock(_data_mutex);
  if (static_cast<size_t>(offset + number_of_bytes_to_write) > _data.size())
    _data.resize(static_cast<size_t>(offset + number_of_bytes_to_write));

  spdlog::info(L"Write {} : NumberOfBytesToWrite {} Offset {}", get_filename(),
               number_of_bytes_to_write, offset);
  memcpy(&_data[static_cast<size_t>(offset)], buffer, number_of_bytes_to_write);
  return number_of_bytes_to_write;
}

const LONGLONG filenode::get_filesize() {
  std::lock_guard<std::mutex> lock(_data_mutex);
  return static_cast<LONGLONG>(_data.size());
}

void filenode::set_endoffile(const LONGLONG& byte_offset) {
  std::lock_guard<std::mutex> lock(_data_mutex);
  _data.resize(static_cast<size_t>(byte_offset));
}

const std::wstring filenode::get_filename() {
  std::lock_guard<std::mutex> lock(_fileName_mutex);
  return _fileName;
}

void filenode::set_filename(const std::wstring& f) {
  std::lock_guard<std::mutex> lock(_fileName_mutex);
  _fileName = f;
}

void filenode::add_stream(const std::shared_ptr<filenode>& stream) {
  std::lock_guard<std::mutex> lock(_data_mutex);
  _streams[stream->get_filename()] = stream;
}

void filenode::remove_stream(const std::shared_ptr<filenode>& stream) {
  std::lock_guard<std::mutex> lock(_data_mutex);
  _streams.erase(stream->get_filename());
}

std::unordered_map<std::wstring, std::shared_ptr<filenode>> filenode::get_streams() {
  std::lock_guard<std::mutex> lock(_data_mutex);
  return _streams;
}

//---------------------------------------------------------------------------------------
typedef int(__stdcall* fn_get_file_security)(const wchar_t* file_name,
                                  /*PSECURITY_INFORMATION*/ void* security_information,
                                  /*PSECURITY_DESCRIPTOR*/ void* security_descriptor,
                                  uint32_t buffer_length,
                                  uint32_t* length_needed,
                                  iris_vfs_file_info_ptr file_info);

typedef int(__stdcall* fn_set_file_security)(const wchar_t* file_name,
                                  /*PSECURITY_INFORMATION*/ void* security_information,
                                  /*PSECURITY_DESCRIPTOR*/ void* security_descriptor,
                                  uint32_t buffer_length,
                                  iris_vfs_file_info_ptr file_info);
iris_vfs_operations memfs_operations = {memfs_createfile,
                                        memfs_cleanup,
                                        memfs_closeFile,
                                        memfs_readfile,
                                        memfs_writefile,
                                        memfs_flushfilebuffers,
                                        memfs_getfileInformation,
                                        memfs_findfiles,
                                        nullptr, // FindFilesWithPattern
                                        memfs_setfileattributes,
                                        memfs_setfiletime,
                                        memfs_deletefile,
                                        memfs_deletedirectory,
                                        memfs_movefile,
                                        memfs_setendoffile,
                                        memfs_setallocationsize,
                                        memfs_lockfile,
                                        memfs_unlockfile,
                                        memfs_getdiskfreespace,
                                        memfs_getvolumeinformation,
                                        memfs_mounted,
                                        memfs_unmounted,
                                        (fn_get_file_security)memfs_getfilesecurity,
                                        (fn_set_file_security)memfs_setfilesecurity,
                                        memfs_findstreams};
} // namespace memfs

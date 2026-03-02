#pragma once

namespace iris {

#ifdef __clang__
#define __forceinline inline __attribute__((__always_inline__))
#endif

#if defined(_MSC_VER)
extern "C" long  _InterlockedIncrement(long volatile* Addend);
#pragma intrinsic (_InterlockedIncrement)
extern "C" long _InterlockedDecrement(long volatile* Addend);
#pragma intrinsic (_InterlockedDecrement)
extern "C" long _InterlockedCompareExchange(long volatile* Dest, long Exchange, long Comp);
#pragma intrinsic (_InterlockedCompareExchange)
#endif

	inline int atomic_increment(int* i32)
	{
#if defined(__clang__) || defined(__GNUC__)
		return __sync_add_and_fetch(i32, 1);
#elif defined(_MSC_VER)
		static_assert(sizeof(long) == sizeof(int), "unexpected size");
		return _InterlockedIncrement((volatile long*)i32);
#elif defined(__GNUC__)
		int result;
		__asm__ __volatile__("lock; xaddl %0, %1"
			: "=r" (result), "=m" (*i32)
			: "0" (1), "m" (*i32)
			: "memory"
		);
		return result + 1;
#else
		return ++*i32;
#endif
	}
	inline int atomic_decrement(int* i32)
	{
#if defined(__clang__) || defined(__GNUC__)
		return __sync_add_and_fetch(i32, -1);
#elif defined(_MSC_VER)
		return _InterlockedDecrement((volatile long*)i32);
#elif defined(__GNUC__)
		int32 result;
		__asm__ __volatile__("lock; xaddl %0, %1"
			: "=r" (result), "=m" (*i32)
			: "0" (-1), "m" (*i32)
			: "memory"
		);
		return result - 1;
#else
		return --*i32;
#endif
	}
	inline bool atomic_cas(int* i32, int newValue, int condition)
	{
#if defined(__clang__) || defined(__GNUC__)
		return __sync_bool_compare_and_swap(i32, condition, newValue);
#elif defined(_MSC_VER)
		return ((long)_InterlockedCompareExchange((volatile long*)i32, (long)newValue, (long)condition) == condition);
#elif defined(__GNUC__)
		int result;
		__asm__ __volatile__(
			"lock; cmpxchgl %3, (%1) \n"                    // Test *i32 against EAX, if same, then *i32 = newValue 
			: "=a" (result), "=r" (i32)                     // outputs
			: "a" (condition), "r" (newValue), "1" (i32)    // inputs
			: "memory"                                      // clobbered
		);
		return result == condition;
#else
		if (*i32 == condition)
		{
			*i32 = newValue;
			return true;
		}
		return false;
#endif
	}

    struct ref_counted 
	{
		ref_counted() {}
		virtual ~ref_counted() {}

		int retain_internal()
		{
			return atomic_increment(&ref_int_);
		}
		int release_internal()
		{
			auto c = atomic_decrement(&ref_int_);
			if (ref_int_ == 0)
			{
				delete this;
			}
			return c;
		}
		int release()
		{
			auto c = atomic_decrement(&ref_ext_);
			if (ref_ext_ == 0)
			{
				release_internal();
			}
			return c;
		}
		int retain()
		{
			return atomic_increment(&ref_ext_);
		}
	private:
		int ref_int_ = 1;
		int ref_ext_ = 1;
	};

	template <class T>
	class ref_ptr
	{
	public:
		explicit ref_ptr(T * pObj) : ptr_(pObj) {}
		ref_ptr(ref_ptr<T> const& Other)
			: ptr_(Other.ptr_) {
			if (ptr_)
				ptr_->retain();
		}
		ref_ptr() : ptr_(nullptr) {}
		~ref_ptr()
		{
			if (ptr_)
			{
				ptr_->release();
				ptr_ = nullptr;
			}
		}
		__forceinline T& operator*() const { { return *ptr_; } }
		__forceinline T* operator->() const { { return ptr_; } }
		explicit operator bool() const
		{
			return ptr_ != nullptr;
		}
		void swap(ref_ptr& Other)
		{
			T * const pValue = Other.ptr_;
			Other.ptr_ = ptr_;
			ptr_ = pValue;
		}
		ref_ptr& operator=(const ref_ptr& Other) {
			typedef ref_ptr<T> ThisType;
			if (&Other != this) {
				ThisType(Other).swap(*this);
			}
			return *this;
		}
		T* get() const { return ptr_; }
		T** getAddressOf() { return &ptr_;}
	private:
		T* ptr_;
	};

	
	template <class T>
    class weak_ptr {
    public:
      explicit weak_ptr(T* pObj) : ptr_(pObj) {}
      weak_ptr(weak_ptr<T> const& Other) : ptr_(Other.ptr_) {
        if (ptr_)
          ptr_->retain_internal();
      }
      weak_ptr() : ptr_(nullptr) {}
      ~weak_ptr() {
        if (ptr_) {
          ptr_->release_internal();
          ptr_ = nullptr;
        }
      }
      __forceinline T& operator*() const {
        return *ptr_;
      }
      __forceinline T* operator->() const {
		return ptr_;
      }
      explicit operator bool() const { return ptr_ != nullptr; }
      void swap(weak_ptr& Other) {
        T* const pValue = Other.ptr_;
        Other.ptr_ = ptr_;
        ptr_ = pValue;
      }
      weak_ptr& operator=(const weak_ptr& Other) {
        typedef weak_ptr<T> ThisType;
        if (&Other != this) {
          ThisType(Other).swap(*this);
        }
        return *this;
      }
      T* get() const { return ptr_; }
      T** getAddressOf() { return &ptr_; }

    private:
      T* ptr_;
    };
    }
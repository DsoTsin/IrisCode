/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree.
 */

#ifndef _BOM_H
#define _BOM_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Memory Management */

struct bom_context_memory {
    void *data;
    size_t size;

    void (*resize)(struct bom_context_memory *memory, size_t size);
    void (*free)(struct bom_context_memory *memory);
    void *ctx;
};

struct bom_context_memory
bom_context_memory(void const *data, size_t size);

struct bom_context_memory
bom_context_memory_file(const char *fn, bool writeable, size_t minimum_size);

struct bom_context * bom_read_file(const char *file_name);

struct bom_context;

struct bom_context *
bom_alloc_empty(struct bom_context_memory memory);

struct bom_context *
bom_alloc_load(struct bom_context_memory memory);

struct bom_context_memory const *
bom_memory(struct bom_context const *context);

void
bom_free(struct bom_context *context);


/* Index */

typedef bool (*bom_index_iterator)(struct bom_context *context, uint32_t index, void *data, size_t data_len, void *ctx);

void
bom_index_iterate(struct bom_context *context, bom_index_iterator iterator, void *ctx);

void *
bom_index_get(struct bom_context *context, uint32_t index, size_t *data_len);

void
bom_index_reserve(struct bom_context *context, size_t count);

uint32_t
bom_index_add(struct bom_context *context, const void *data, size_t data_len);

uint32_t
bom_free_indices_add(struct bom_context *context, size_t count);

void
bom_index_append(struct bom_context *context, uint32_t index, size_t data_len);


/* Variable */

typedef bool (*bom_variable_iterator)(struct bom_context *context, const char *name, int data_index, void *ctx);

void
bom_variable_iterate(struct bom_context *context, bom_variable_iterator iterator, void *ctx);

int
bom_variable_get(struct bom_context *context, const char *name);

void*
bom_variable_get_by_name(struct bom_context *context, const char *name, size_t *data_len);

void
bom_variable_add(struct bom_context *context, const char *name, int data_index);


/* Tree */

struct bom_tree_context;

struct bom_tree_context *
bom_tree_alloc_empty(struct bom_context *context, const char *variable_name);

struct bom_tree_context *
bom_tree_alloc_load(struct bom_context *context, const char *variable_name);

void
bom_tree_free(struct bom_tree_context *tree);

struct bom_context *
bom_tree_context_get(struct bom_tree_context *tree);

const char *
bom_tree_variable_get(struct bom_tree_context *tree);

typedef void (*bom_tree_iterator)(struct bom_tree_context *tree, void *key, size_t key_len, void *value, size_t value_len, void *ctx);

void
bom_tree_iterate(struct bom_tree_context *tree, bom_tree_iterator iterator, void *ctx);

void
bom_tree_reserve(struct bom_tree_context *tree, size_t count);

void
bom_tree_add(struct bom_tree_context *tree, const void *key, size_t key_len, const void *value, size_t value_len);


#ifdef __cplusplus
}

#include <string>

namespace bom {
class Tree;

class Context {
public:
    explicit Context(const char* filename);
    ~Context() {
        if (_context) {
            bom_free(_context);
            _context = nullptr;
        }
    }

    void reserve_index(size_t count) {
      if (_context) {
        bom_index_reserve(_context, count);
      }
    }

    void* index_get(uint32_t index, size_t& len) const {
      if (_context) {
        return bom_index_get(_context, index, &len);
      } else {
        return nullptr;
      }
    }

    int variable_get(const char* var_name) const {
      if (_context) {
        return bom_variable_get(_context, var_name);
      }
      return -1;
    }

    template <class T>
    const T* get(const char* var_name, size_t& size) const {
      int index = variable_get(var_name);
      return reinterpret_cast<const T*>(index_get(index, size));
    }

    template <class F>
    void iterate(F fn, const char* var_name);

    virtual Tree* makeTree(const char* var_name);
    virtual Tree* loadTree(const char* var_name) const;

private:
    bom_context* _context;
};

class Tree {
public:
  ~Tree() {
    if (_treeContext) {
      bom_tree_free(_treeContext);
      _treeContext = nullptr;
    }
  }

  void reserve(size_t count)
  {
    if (_treeContext)
    {
      bom_tree_reserve(_treeContext, count);
    }
  }

  void add(const std::string_view& key, const std::string_view& value) {
    if (_treeContext) {
      bom_tree_add(_treeContext, key.data(), key.size(), value.data(), value.size());
    }
  }

  virtual void onIterate(const std::string_view& key, const std::string_view& value) {}

  void iterate() {
    if (_treeContext) {
      bom_tree_iterate(_treeContext, &Tree::_iterate, this);
    }
  }

  const char* variable() {
      return bom_tree_variable_get(_treeContext);
  }

  explicit Tree(bom_tree_context* treeContext) : _treeContext(treeContext) {}

private:
  static void _iterate(struct bom_tree_context* tree,
                       void* key,
                       size_t key_len,
                       void* value,
                       size_t value_len,
                       void* ctx) {
    Tree* treeContext = (Tree*)ctx;
    treeContext->onIterate(std::string_view((const char*)key, key_len),
                           std::string_view((const char*)value, value_len));
  }

  bom_tree_context* _treeContext;
};

inline Context::Context(const char* filename) : _context(nullptr) {
  auto memory = bom_context_memory_file(filename, false, 0);
 _context =  bom_alloc_load(memory);
}

inline Tree* Context::makeTree(const char* var_name) {
  auto treeContext = bom_tree_alloc_empty(_context, var_name);
  return new Tree(treeContext);
}
inline Tree* Context::loadTree(const char* var_name) const {
  auto treeContext = bom_tree_alloc_load(_context, var_name);
  return new Tree(treeContext);
}
template <class F>
inline void Context::iterate(F fn, const char* var_name) {
  auto tree = bom_tree_alloc_load(_context, var_name);;
  if (tree) {
    bom_tree_iterate(
        tree,
        [](struct bom_tree_context* tree, void* key, size_t key_len, void* value, size_t value_len,
           void* ctx) { 
                F& f = *(F*)ctx;
          f(std::string_view((const char*)key, key_len),
            std::string_view((const char*)value, value_len));
        }, &fn);
    bom_tree_free(tree);
  }
}
} // namespace bom

#endif

#endif /* _BOM_H */

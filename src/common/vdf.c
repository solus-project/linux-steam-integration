/*
 * This file is part of linux-steam-integration.
 *
 * Copyright © 2016-2017 Ikey Doherty <ikey@solus-project.com>
 *
 * linux-steam-integration is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"
#include "vdf.h"

#define VDF_CHAR_QUOTE '"'
#define VDF_CHAR_SLASH '\\'
#define VDF_CHAR_OPEN '{'
#define VDF_CHAR_CLOSE '}'

typedef enum {
        VDF_FLAG_MIN = 1 << 0,
        VDF_FLAG_QUOTED = 1 << 1,        /**< Inside a quoted block */
        VDF_FLAG_COMMENT = 1 << 2,       /**< Inside a comment block */
        VDF_FLAG_BLOCK_COMMENT = 1 << 3, /**< Inside a multiline comment block */
        VDF_FLAG_CHEW_WHITESPACE = 1 << 4,
} VdfFlags;

/**
 * VdfFile is the main dude required for parsing a file
 */
struct VdfFile {
        /* Input */
        size_t buffer_len;
        char *buffer;

        /* Output (memstream) */
        FILE *ring;
        char *out_buffer;
        size_t out_len;

        /* Input file */
        int fd;

        /* Tracking */
        size_t index;
        int flags;
        char *key_id;
        int n_quote; /**only allow 2 sets per "line" */
        VdfNode *section;
        VdfNode *root;

        bool error; /**< Allow breaking. */
};

/**
 * Function pointer to the generic handler functions.
 */
typedef bool (*vdf_handle_func)(VdfFile *file, char c);

/**
 * Free node
 */
static void vdf_node_free(VdfNode *node)
{
        if (!node) {
                return;
        }
        if (node->key) {
                free(node->key);
        }
        if (node->value) {
                free(node->value);
        }

        vdf_node_free(node->sibling);
        vdf_node_free(node->child);
        free(node);
}

/**
 * Allocate a new VdfNode, copying the key and value if they exist.
 */
static VdfNode *vdf_node_new(const char *key, const char *value)
{
        VdfNode *ret = NULL;

        ret = calloc(1, sizeof(VdfNode));
        if (!ret) {
                return NULL;
        }
        if (key) {
                ret->key = strdup(key);
                if (!ret->key) {
                        goto failed;
                }
        }
        if (value) {
                ret->value = strdup(value);
                if (!ret->value) {
                        goto failed;
                }
        }

        return ret;
failed:
        vdf_node_free(ret);
        return NULL;
}

void vdf_file_close(VdfFile *file)
{
        if (!file) {
                return;
        }

        if (file->buffer) {
                munmap(file->buffer, file->buffer_len);
        }
        if (file->fd > -1) {
                close(file->fd);
        }
        if (file->ring) {
                fclose(file->ring);
        }
        if (file->out_buffer) {
                free(file->out_buffer);
        }
        if (file->key_id) {
                free(file->key_id);
        }
        /* TODO: Make free separate from close, so we can just keep the node */
        vdf_node_free(file->root);
        free(file);
}

VdfFile *vdf_file_open(const char *path)
{
        VdfFile *ret = NULL;
        struct stat st = { 0 };

        ret = calloc(1, sizeof(VdfFile));
        if (!ret) {
                return NULL;
        }
        ret->fd = -1;

        ret->fd = open(path, O_RDONLY);
        if (ret->fd < 0) {
                goto fail;
        }

        if (fstat(ret->fd, &st) != 0) {
                goto fail;
        }

        ret->buffer_len = (size_t)st.st_size;

        ret->buffer = mmap(NULL, ret->buffer_len, PROT_READ, MAP_PRIVATE, ret->fd, 0);
        if (!ret->buffer) {
                goto fail;
        }

        /* Now we have input map, build output memstream */
        ret->ring = open_memstream(&ret->out_buffer, &ret->out_len);
        if (!ret->ring) {
                goto fail;
        }

        return ret;
fail:
        vdf_file_close(ret);
        return NULL;
}

VdfNode *vdf_file_get_root(VdfFile *file)
{
        if (!file) {
                return NULL;
        }
        return file->root;
}

/**
 * Advance the buffer pointer forward if possible.
 */
static char vdf_file_next(VdfFile *file)
{
        file->index++;
        if (file->index > file->buffer_len) {
                file->index = file->buffer_len;
                return '\0';
        }
        return file->buffer[file->index];
}

/**
 * Attempt to peek at the next character in the buffer
 */
static char vdf_file_peek(VdfFile *file)
{
        if (file->index + 1 > file->buffer_len) {
                return '\0';
        }
        return file->buffer[file->index + 1];
}

/**
 * Skip a character in the stream
 */
static void vdf_file_skip(VdfFile *file)
{
        if (++file->index > file->buffer_len) {
                file->index = file->buffer_len;
        }
}

/**
 * Push a char to the ring buffer
 */
static bool vdf_file_push_ring(VdfFile *file, char c)
{
        if (fwrite(&c, 1, sizeof(c), file->ring) != 1) {
                return false;
        }
        return true;
}

/**
 * End the ring buffer by writing a NUL byte
 */
static bool vdf_file_end_ring(VdfFile *file)
{
        if (!vdf_file_push_ring(file, '\0')) {
                return false;
        }
        if (fflush(file->ring) != 0) {
                return false;
        }
        return true;
}

/**
 * Reset the ring once more to begin writing again.
 */
static bool vdf_file_reset_ring(VdfFile *file)
{
        if (fflush(file->ring) != 0) {
                return false;
        }

        if (fseek(file->ring, 0, SEEK_SET) != 0) {
                return false;
        }
        return true;
}

/**
 * Handle newlines
 *
 * When encountering a newline we need to do a bit of state cleanup
 */
static bool vdf_file_handle_newline(VdfFile *file, char c)
{
        if (c != '\n') {
                return false;
        }
        file->flags &= ~VDF_FLAG_COMMENT;
        if (file->flags & VDF_FLAG_QUOTED) {
                file->flags |= VDF_FLAG_CHEW_WHITESPACE;
        } else {
                file->flags &= ~VDF_FLAG_CHEW_WHITESPACE;
        }
        return false;
}

/**
 * Handle quote characters
 *
 * This function deals with the start/end quotes encountered in the stream
 * to set keys and values
 */
static bool vdf_file_handle_quote(VdfFile *file, char c)
{
        if (c != VDF_CHAR_QUOTE) {
                return false;
        }

        /* Start tracking quoting if necessary */
        if (!(file->flags & VDF_FLAG_QUOTED)) {
                /* Start tracking the quoted thing */
                if (file->n_quote > 2) {
                        lsi_log_error("vdf: Cannot start a third section");
                        goto bail;
                }
                file->flags |= VDF_FLAG_QUOTED;
                file->flags &= ~VDF_FLAG_CHEW_WHITESPACE;
                vdf_file_reset_ring(file);
                return true;
        }

        ++file->n_quote;

        /* Stop tracking the quoted thing */
        file->flags ^= VDF_FLAG_QUOTED;
        vdf_file_end_ring(file);

        /* Capture the ID */
        if (file->n_quote == 1) {
                if (file->key_id) {
                        lsi_log_error("vdf: Key should not already be set!");
                        goto bail;
                }
                file->key_id = strdup(file->out_buffer);
                return true;
        }

        if (!file->key_id) {
                lsi_log_error("vdf: Missing key for value!");
                goto bail;
        }

        if (file->n_quote != 2) {
                lsi_log_error("vdf: Invalid number of quotes on line");
                goto bail;
        }

        /* Reset n_quote now */
        file->n_quote = 0;

        /* Store the node now */
        VdfNode *t = vdf_node_new(file->key_id, file->out_buffer);
        if (!t) {
                fputs("OOM\n", stderr);
                goto bail;
        }

        /* dispose of key ID now */
        free(file->key_id);
        file->key_id = NULL;
        file->flags &= ~VDF_FLAG_CHEW_WHITESPACE;

        /* Store the new key->value mapping */
        t->parent = file->section->parent;
        t->sibling = file->section->child;
        file->section->child = t;

        return true;

bail:
        file->error = true;
        return true;
}

/**
 * Handle opening of a section
 *
 * This will perform some basic validation, and when all conditions are satisfied
 * we'll attempt construction of a new section
 */
static bool vdf_file_handle_section_open(VdfFile *file)
{
        VdfNode *node = NULL;
        vdf_file_reset_ring(file);

        if (file->n_quote == 2) {
                lsi_log_error("vdf: section cannot have value!");
                goto bail;
        }

        if (file->n_quote != 1) {
                lsi_log_error("vdf: Section is missing id!");
                goto bail;
        }

        file->n_quote = 0;

        if (!file->key_id) {
                lsi_log_error("vdf: key_id should not be NULL!");
                goto bail;
        }

        node = vdf_node_new(file->out_buffer, NULL);
        if (!node) {
                goto bail;
        }
        free(file->key_id);
        file->key_id = NULL;

        node->parent = file->section;
        node->sibling = file->section->child;
        file->section->child = node;

        file->section = node;
        return true;

bail:
        file->error = true;
        return true;
}

/**
 * Handle closing of a section
 *
 * This will perform some basic error checking to ensure we're closing it
 * correctly. At this point we'll wind back to the nodes parent
 */
static bool vdf_file_handle_section_close(VdfFile *file)
{
        if (file->n_quote != 0) {
                lsi_log_error("vdf: unterminated section!");
                goto bail;
        }

        if (!file->section) {
                lsi_log_error("vdf: Closed section without creating one!");
                goto bail;
        }

        /* Wind back */
        file->section = file->section->parent;
        if (!file->section) {
                goto bail;
        }
        return true;

bail:
        file->error = true;
        return true;
}

/**
 * Handle brace characters
 *
 * This function will proxy to the relevant handler function
 */
static bool vdf_file_handle_section(VdfFile *file, char c)
{
        switch (c) {
        case VDF_CHAR_CLOSE:
                return vdf_file_handle_section_close(file);
        case VDF_CHAR_OPEN:
                return vdf_file_handle_section_open(file);
        default:
                return false;
        }
}

/**
 * Handle normal text
 *
 * Whilst in QUOTED mode we'll keep pushing text to the buffer, which will
 * later become keys and values.
 *
 * We'll also do some sanity checks here to escape any common text sequences,
 * so as not to break the quotes
 */
static bool vdf_file_handle_text(VdfFile *file, char c)
{
        char next = '\0';

        /* Must be within quoting */
        if (!(file->flags & VDF_FLAG_QUOTED)) {
                return false;
        }

        if (file->flags & VDF_FLAG_CHEW_WHITESPACE && isspace(c)) {
                return true;
        }

        file->flags &= ~VDF_FLAG_CHEW_WHITESPACE;

        /* Check if we need to do any escaping */
        if (c != VDF_CHAR_SLASH) {
                vdf_file_push_ring(file, c);
                return true;
        }

        /* Perform text escaping */
        switch ((next = vdf_file_peek(file))) {
        case 'r':
                vdf_file_push_ring(file, '\r');
                vdf_file_skip(file);
                return true;
        case 'n':
                vdf_file_push_ring(file, '\n');
                vdf_file_skip(file);
                return true;
        case 't':
                vdf_file_push_ring(file, '\t');
                vdf_file_skip(file);
                return true;
        case '"':
                vdf_file_push_ring(file, '"');
                vdf_file_skip(file);
                return true;
        case '\'':
                vdf_file_push_ring(file, '\'');
                vdf_file_skip(file);
                return true;
        case '\\':
                vdf_file_push_ring(file, '\\');
                vdf_file_skip(file);
                return true;
        default:
                // TODO: Handle errors properly!
                lsi_log_error("vdf: Invalid escape sequence '\\%c'", next);
                file->error = true;
                return true;
        }
}

/**
 * Handle multiline comments
 *
 * This method will attempt to look for the start and ends of multiline comment
 * blocks, which will result in all text from the start until the trailing
 * characters of the comment to be ignored.
 */
static bool vdf_file_handle_multiline_comment(VdfFile *file, char c)
{
        if (c == '/' && vdf_file_peek(file) == '*') {
                vdf_file_skip(file);
                if (file->flags & VDF_FLAG_BLOCK_COMMENT) {
                        lsi_log_error("vdf: Starting nested block comment");
                        goto bail;
                }
                file->flags |= VDF_FLAG_BLOCK_COMMENT;
        } else if (c == '*' && vdf_file_peek(file) == '/') {
                vdf_file_skip(file);
                if (!(file->flags & VDF_FLAG_BLOCK_COMMENT)) {
                        lsi_log_error("vdf: Ended comment without starting one");
                        goto bail;
                }
                file->flags ^= VDF_FLAG_BLOCK_COMMENT;
        } else {
                return false;
        }

        return true;

bail:
        file->error = true;
        return true;
}

/**
 * Handle single comments
 *
 * This will handle C++ style '//' comments and cause the remaining text on
 * the line to be ignored
 */
static bool vdf_file_handle_single_comment(VdfFile *file, char c)
{
        /* Don't switch us on if we're inside a multiline comment! */
        if (file->flags & VDF_FLAG_BLOCK_COMMENT) {
                return false;
        }

        /* Must be a C++ style comment. */
        if (!(c == '/' && vdf_file_peek(file) == '/')) {
                return false;
        }

        /* Skip the next character, and turn on comment mode */
        vdf_file_skip(file);
        file->flags |= VDF_FLAG_COMMENT;
        return true;
}

/**
 * Handle comment state
 *
 * If we're in a comment we might need to skip this line entirely, however
 * if its a multiline comment we need to preemptively evaluate the termination.
 */
static bool vdf_file_handle_commented(VdfFile *file, char c)
{
        if (file->flags & VDF_FLAG_COMMENT) {
                return true;
        }
        if (file->flags & VDF_FLAG_BLOCK_COMMENT) {
                /* Always return true here, must ignore the line! */
                vdf_file_handle_multiline_comment(file, c);
                return true;
        }
        return false;
}

bool vdf_file_parse(VdfFile *file)
{
        char c = file->buffer[0];
        bool ret = false;
        static vdf_handle_func funcs[] = {
                &vdf_file_handle_newline,
                &vdf_file_handle_commented,
                &vdf_file_handle_quote,
                &vdf_file_handle_text,
                &vdf_file_handle_section,
                &vdf_file_handle_single_comment,
                &vdf_file_handle_multiline_comment,
        };
        static size_t n_funcs = sizeof(funcs) / sizeof(funcs[0]);

        file->flags = 0;
        file->key_id = NULL;
        file->section = file->root = vdf_node_new(NULL, NULL);
        if (!file->root) {
                goto bail;
        }

        while (c != '\0') {
                /* Pass through all of our functions */
                for (size_t i = 0; i < n_funcs; i++) {
                        if (funcs[i](file, c)) {
                                goto next;
                        }
                }
                if (file->error) {
                        goto bail;
                }

                /* Now the only thing left is to allow whitespace */
                if (!isspace(c)) {
                        lsi_log_error("vdf: Illegal character in stream: '%c'", c);
                        goto bail;
                }
        next:
                c = vdf_file_next(file);
        }

        ret = true;

        /* Check a comment actually ended .. */
        if (file->flags & VDF_FLAG_BLOCK_COMMENT) {
                lsi_log_error("vdf: Started block comment without ending one");
                ret = false;
        }

bail:
        if (file->key_id) {
                free(file->key_id);
                file->key_id = NULL;
        }
        return ret;
}

VdfNode *vdf_node_get_child(VdfNode *node, const char *id)
{
        if (!id || !node) {
                return NULL;
        }

        for (VdfNode *tmp = node->child; tmp; tmp = tmp->sibling) {
                if (tmp->key && strcmp(tmp->key, id) == 0) {
                        return tmp;
                }
        }
        return NULL;
}

VdfNode *vdf_node_get(VdfNode *node, ...)
{
        va_list va;
        va_start(va, node);
        char *id = NULL;
        VdfNode *retr = node;

        while ((id = va_arg(va, char *)) != NULL) {
                retr = vdf_node_get_child(retr, id);
                if (!retr) {
                        goto end;
                }
        }

end:
        va_end(va);
        return retr;
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 expandtab:
 * :indentSize=8:tabSize=8:noTabs=true:
 */

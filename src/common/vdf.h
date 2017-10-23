/*
 * This file is part of linux-steam-integration.
 *
 * Copyright © 2016-2017 Ikey Doherty <ikey@solus-project.com>
 *
 * linux-steam-integration is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 */

#pragma once

#include <stdbool.h>
#include <stdlib.h>

#define _GNU_SOURCE

/**
 * VdfFile is an opaque type containing fields used primarily for parsing
 * a .vdf file
 *
 * A VdfFile is used to parse the Valve Data Format file, allowing LSI to
 * parse the users library paths, etc.
 */
typedef struct VdfFile VdfFile;

/**
 * A VdfNode may be any element in a VDF document, and has optional value,
 * children and parent.
 * The root node in a document has no parent
 */
typedef struct VdfNode {
        struct VdfNode *sibling; /**First sibling */
        struct VdfNode *child;   /**<First child, may have siblings. */
        struct VdfNode *parent;  /**<Parent if not the root */

        char *key;   /**<Only null for the root node */
        char *value; /**<Only exists for key-value nodes */
} VdfNode;

/**
 * Close and free an existing VdfFile
 */
void vdf_file_close(VdfFile *file);

/**
 * Attempt to map the source path and open a .vdf file
 */
VdfFile *vdf_file_open(const char *path);

/**
 * Return the root node for this file, if it has been parsed
 */
VdfNode *vdf_file_get_root(VdfFile *file);

/**
 * Attempt to parse a file
 *
 * @file Pointer to a VdfFile instance
 * @returns true if parsing succeeds
 */
bool vdf_file_parse(VdfFile *file);

/**
 * Grab a child node by the given ID
 *
 * @node Parent node
 * @id Child ID
 */
VdfNode *vdf_node_get_child(VdfNode *node, const char *id);

/**
 * Variadic function to retrieve a child node
 *
 * This function will keep iterating the args passed until it hits NULL and
 * has a node, or no node can be found prior to this and it returns NULL.
 */
__attribute__((sentinel(0))) VdfNode *vdf_node_get(VdfNode *node, ...);

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

/*
 * This file is part of libnica.
 *
 * Copyright (C) 2016 Intel Corporation
 *
 * libnica is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#pragma once

#include "hashmap.h"
#include "util.h"

/**
 * The Nica INI Parser focuses on simplicity and ease of use, therefore it
 * currently only supports a read-only parse operation.
 *
 * The parse functions are used to return a _root_ @NcHashmap which contains
 * name to section mappings. Each section is in turn an @NcHashmap with string
 * to string mappings.
 *
 * Consider the following example:
 *
 *      [Person]
 *      alive = true
 *      ; This person is alive
 *
 * The returned @NcHashmap will contain the key "Person", which corresponds to
 * another @NcHashmap. This @NcHashmap will contain the key "alive", which has
 * the value "true". Therefore we can access this variable using the @NcHashmap
 * API:
 *
 *      char *alive = nc_hashmap_get(nc_hashmap_get(ini, "Person"), "alive");
 *
 * Conversely, we can use the API to determine whether a section or key is set:
 *
 *      if (nc_hashmap_contains(ini, "Person"))
 *
 * As @NcHashmap returns NULL in nc_hashmap_get, you can happily pass NULL
 * keys, allowing quick key fetches without checking if the section exists.
 *
 * In our INI file, lines beginning with ";" or "#" are considered as comments
 * and are not processed. A line following the "key = value" notation is an
 * _assignment_. "[Section]" is considered to be a section in the INI file, and
 * all INI files must have at least one section.
 *
 * We do allow duplication section definitions, by following a merge policy. If
 * a key is redefined then the original key->value mapping is freed and the new
 * key->value mapping is inserted.
 *
 * The parser will return a correctly configured @NcHashmap, the only cleanup
 * required is to call nc_hashmap_free on it. Alternatively, use the autofree
 * helper, for a RAII approach:
 *
 *      autofree(NcHashmap) *map = nc_ini_file_parse("myfile.ini");
 */

/**
 * Errors that are reported when parsing an ini file
 */
typedef enum {
        NC_INI_ERROR_MIN = 0,
        NC_INI_ERROR_FILE,         /**< File based error, check strerror(errno) */
        NC_INI_ERROR_EMPTY_KEY,    /**< Empty key in an assignment line */
        NC_INI_ERROR_NOT_CLOSED,   /**< Encountered section start that wasn't closed with a ']' */
        NC_INI_ERROR_NO_SECTION,   /**< Key assignment with no defined sections */
        NC_INI_ERROR_INVALID_LINE, /**< Encountered an invalid line (syntax) */
        NC_INI_ERROR_MAX
} NcIniError;

/**
 * Convenience wrapper for nc_ini_file_parse_full, reports errors to stderrr
 * and returns an NcHashmap if the parsing succeeded
 */
NcHashmap *nc_ini_file_parse(const char *path);

/**
 * Parse an INI file into a hash of hashes.
 *
 * @param path Path on the filesystem to parse, must be in INI syntax.
 * @param out_map Pointer to store the resulting root NcHashmap in
 * @param error_line_number If not null, then the erronous line number is
 * stored.
 *
 * @return 0 if the call was succesful, or a negative integer. See nc_ini_error
 */
int nc_ini_file_parse_full(const char *path, NcHashmap **out_map,
                                         int *error_line_number);

/**
 * Return a string representation for a given @NcIniError
 *
 * @param error Valid NcIniError code
 * @return a static string owned by the implementation
 */
const char *nc_ini_error(NcIniError error);

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

/*
	pev - the PE file analyzer toolkit

	peres.c - retrive informations and binary data of resources

	Copyright (C) 2012 - 2020 pev authors

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	In addition, as a special exception, the copyright holders give
	permission to link the code of portions of this program with the
	OpenSSL library under certain conditions as described in each
	individual source file, and distribute linked combinations
	including the two.
	
	You must obey the GNU General Public License in all respects
	for all of the code used other than OpenSSL.  If you modify
	file(s) with this exception, you may extend this exception to your
	version of the file(s), but you are not obligated to do so.  If you
	do not wish to do so, delete this exception statement from your
	version.  If you delete this exception statement from all source
	files in the program, then also delete it here.
*/

#include "common.h"
#include <libpe/utlist.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define PROGRAM "peres"

const char *g_resourceDir = "resources";

typedef struct {
	bool all;
	bool extract;
	bool namedExtract;
	bool info;
	bool statistics;
	bool list;
	bool version;
	bool help;
} options_t;

static void usage(void)
{
	static char formats[255];
	output_available_formats(formats, sizeof(formats), '|');
	printf("Usage: %s OPTIONS FILE\n"
		"Show information about resource section and extract it\n"
		"\nExample: %s -a putty.exe\n"
		"\nOptions:\n"
		" -a, --all                              Show all information, statistics and extract resources\n"
		" -f, --format <%s>  change output format (default: text)\n"
		" -i, --info                             Show resources information\n"
		" -l, --list                             Show list view\n"
		" -s, --statistics                       Show resources statistics\n"
		" -x, --extract                          Extract resources\n"
		" -X, --named-extract                    Extract resources with path names\n"
		" -v, --file-version                     Show File Version from PE resource directory\n"
		" -V, --version                          Show version and exit\n"
		" --help                                 Show this help and exit\n",
		PROGRAM, PROGRAM, formats);
}

static void free_options(options_t *options)
{
	if (options == NULL)
		return;

	free(options);
}

static options_t *parse_options(int argc, char *argv[])
{
	options_t *options = malloc_s(sizeof(options_t));
	memset(options, 0, sizeof(options_t));

	/* Parameters for getopt_long() function */
	static const char short_options[] = "a:f:ilsxXvV";

	static const struct option long_options[] = {
		{ "all",            required_argument,  NULL, 'a' },
		{ "format",         required_argument,  NULL, 'f' },
		{ "info",           no_argument,        NULL, 'i' },
		{ "list",           no_argument,        NULL, 'l' },
		{ "statistics",	    no_argument,        NULL, 's' },
		{ "extract",	    no_argument,        NULL, 'x' },
		{ "named-extract",  no_argument,        NULL, 'X' },
		{ "file-version",   no_argument,        NULL, 'v' },
		{ "version",	    no_argument,        NULL, 'V' },
		{ "help",           no_argument,        NULL,  1  },
		{ NULL,             0,                  NULL,  0  }
		};

	int c, ind;

	while ((c = getopt_long(argc, argv, short_options, long_options, &ind)))
	{
		if (c < 0)
			break;

		switch (c)
		{
			case 'a':
				options->all = true;
				break;
			case 'f':
				if (output_set_format_by_name(optarg) < 0)
					EXIT_ERROR("invalid format option");
				break;
			case 'i':
				options->info = true;
				break;
			case 'l':
				options->list = true;
				break;
			case 's':
				options->statistics = true;
				break;
			case 'x':
				options->extract = true;
				break;
			case 'X':
				options->extract = true;
				options->namedExtract = true;
				break;
			case 'v':
				options->version = true;
				break;
			case 'V':
				printf("%s %s\n%s\n", PROGRAM, TOOLKIT, COPY);
				exit(EXIT_SUCCESS);
			case 1: // --help option
				usage();
				exit(EXIT_SUCCESS);
			default:
				fprintf(stderr, "%s: try '--help' for more information\n", PROGRAM);
				exit(EXIT_FAILURE);
		}
	}

	return options;
}

static void peres_show_node(const pe_resource_node_t *node)
{
	char value[MAX_MSG];

	switch (node->type)
	{
		default:
			LIBPE_WARNING("Invalid node type");
			break;
		case LIBPE_RDT_RESOURCE_DIRECTORY:
		{
			const IMAGE_RESOURCE_DIRECTORY * const resourceDirectory = node->raw.resourceDirectory;

			snprintf(value, MAX_MSG, "Resource Directory / %d", node->dirLevel);
			output("\nNode Type / Level", value);

			snprintf(value, MAX_MSG, "%d", resourceDirectory->Characteristics);
			output("Characteristics", value);

			snprintf(value, MAX_MSG, "%d", resourceDirectory->TimeDateStamp);
			output("Timestamp", value);

			snprintf(value, MAX_MSG, "%d", resourceDirectory->MajorVersion);
			output("Major Version", value);

			snprintf(value, MAX_MSG, "%d", resourceDirectory->MinorVersion);
			output("Minor Version", value);

			snprintf(value, MAX_MSG, "%d", resourceDirectory->NumberOfNamedEntries);
			output("Named entries", value);

			snprintf(value, MAX_MSG, "%d", resourceDirectory->NumberOfIdEntries);
			output("Id entries", value);
			break;
		}
		case LIBPE_RDT_DIRECTORY_ENTRY:
		{
			const IMAGE_RESOURCE_DIRECTORY_ENTRY * const directoryEntry = node->raw.directoryEntry;

			snprintf(value, MAX_MSG, "Directory Entry / %d", node->dirLevel);
			output("\nNode Type / Level", value);

			snprintf(value, MAX_MSG, "%d", directoryEntry->u0.data.NameOffset);
			output("Name offset", value);

			snprintf(value, MAX_MSG, "%d", directoryEntry->u0.data.NameIsString);
			output("Name is string", value);

			snprintf(value, MAX_MSG, "%x", directoryEntry->u1.data.OffsetToDirectory);
			output("Offset to directory", value);

			snprintf(value, MAX_MSG, "%d", directoryEntry->u1.data.DataIsDirectory);
			output("Data is directory", value);
			break;
		}
		case LIBPE_RDT_DATA_STRING:
		{
			const IMAGE_RESOURCE_DATA_STRING_U * const dataString = node->raw.dataString;

			snprintf(value, MAX_MSG, "Data String / %d", node->dirLevel);
			output("\nNode Type / Level", value);

			snprintf(value, MAX_MSG, "%d", dataString->Length);
			output("String len", value);

			char ascii_string[MAX_MSG];
			size_t min_size = pe_utils_min(sizeof(ascii_string), dataString->Length + 1);
			pe_utils_str_widechar2ascii(ascii_string, (const char *)dataString->String, min_size);
			ascii_string[min_size - 1] = '\0'; // Null terminate it.

			snprintf(value, MAX_MSG, "%s", ascii_string);
			output("String", value);
			break;
		}
		case LIBPE_RDT_DATA_ENTRY:
		{
			const IMAGE_RESOURCE_DATA_ENTRY * const dataEntry = node->raw.dataEntry;

			snprintf(value, MAX_MSG, "Data Entry / %d", node->dirLevel);
			output("\nNode Type / Level", value);

			snprintf(value, MAX_MSG, "%x", dataEntry->OffsetToData);
			output("OffsetToData", value);

			snprintf(value, MAX_MSG, "%d", dataEntry->Size);
			output("Size", value);

			snprintf(value, MAX_MSG, "%d", dataEntry->CodePage);
			output("CodePage", value);

			snprintf(value, MAX_MSG, "%d", dataEntry->Reserved);
			output("Reserved", value);
			break;
		}
	}
}

static void peres_show_nodes(pe_ctx_t *ctx, const pe_resource_node_t *node)
{
	if (node == NULL)
		return;

	peres_show_node(node);
		
	peres_show_nodes(ctx, node->childNode);
	peres_show_nodes(ctx, node->nextNode);
}

static void peres_build_node_filename(pe_ctx_t *ctx, const pe_resource_node_t *node, char *output, size_t output_size)
{
	for (uint32_t level = 1; level <= node->dirLevel; level++) {
		char partial_path[MAX_PATH];

		const pe_resource_node_t *dir_entry_node = pe_resource_find_parent_node_by_type_and_level(node, LIBPE_RDT_DIRECTORY_ENTRY, level);
		if (dir_entry_node->raw.directoryEntry->u0.data.NameIsString) {
			const IMAGE_RESOURCE_DATA_STRING_U *string_ptr = LIBPE_PTR_ADD(ctx->cached_data.resources->resource_base_ptr, dir_entry_node->raw.directoryEntry->u0.data.NameOffset);
			if (!pe_can_read(ctx, string_ptr, sizeof(IMAGE_RESOURCE_DATA_STRING_U))) {
				LIBPE_WARNING("Cannot read IMAGE_RESOURCE_DATA_STRING_U");
				return;
			}
			const size_t string_size = pe_utils_min(sizeof(partial_path) - 2, string_ptr->Length); // Decrement 2 because we want an extra space: ' \0';
			pe_utils_str_widechar2ascii(partial_path, (const char *)string_ptr->String, string_size);
			partial_path[string_size] = ' ';
			partial_path[string_size + 1] = '\0';
		} else {
			const pe_resource_entry_info_t *match = pe_resource_entry_info_lookup(dir_entry_node->raw.directoryEntry->u0.data.NameOffset);
			if (level == 1 && match != NULL) {
				snprintf(partial_path, sizeof(partial_path), "%s ", match->name);
			} else {
				snprintf(partial_path, sizeof(partial_path), "%04x ", dir_entry_node->raw.directoryEntry->u0.data.NameOffset);
			}
		}

		strncat(output, partial_path, output_size - strlen(output) - 1);
	}

	size_t length = strlen(output);
	output[length-1] = '\0';
}

static void peres_show_list_node(pe_ctx_t *ctx, const pe_resource_node_t *node)
{
	if (node->type == LIBPE_RDT_DATA_ENTRY) {
		char node_info[MAX_PATH];
		memset(node_info, 0, sizeof(node_info));
		peres_build_node_filename(ctx, node, node_info, sizeof(node_info));
		printf("%s (%d bytes)\n", node_info, node->raw.dataEntry->Size);
	}
}

static void peres_show_list(pe_ctx_t *ctx, const pe_resource_node_t *node)
{
	if (node == NULL)
		return;

	peres_show_list_node(ctx, node);

	peres_show_list(ctx, node->childNode);
	peres_show_list(ctx, node->nextNode);
}

static void peres_save_resource(pe_ctx_t *ctx, const pe_resource_node_t *node, bool namedExtract)
{
	assert(node != NULL);
	assert(node->type == LIBPE_RDT_DATA_ENTRY);
	assert(node->dirLevel == LIBPE_RDT_LEVEL3);

	const IMAGE_RESOURCE_DATA_ENTRY *entry = node->raw.dataEntry;

	const uint64_t raw_data_offset = pe_rva2ofs(ctx, entry->OffsetToData);
	const size_t raw_data_size = entry->Size;
	uint8_t *raw_data_ptr = LIBPE_PTR_ADD(ctx->map_addr, raw_data_offset);
	if (!pe_can_read(ctx, raw_data_ptr, raw_data_size)) {
		// TODO: Should we report something?
		fprintf(stderr, "Attempted to read range [ %p, %p ] which is not within the mapped range [ %p, %lx ]\n",
			(void *)raw_data_ptr, LIBPE_PTR_ADD(raw_data_ptr, raw_data_size),
			ctx->map_addr, ctx->map_end);
		return;
	}

	struct stat statDir;
	if (stat(g_resourceDir, &statDir) == -1)
		mkdir(g_resourceDir, 0700);

	char dirName[100];
	memset(dirName, 0, sizeof(dirName));

	const pe_resource_node_t *folder_node = pe_resource_find_parent_node_by_type_and_level(node, LIBPE_RDT_DIRECTORY_ENTRY, LIBPE_RDT_LEVEL1); // dirLevel == 1 is where Resource Types are defined.
	const pe_resource_entry_info_t *entry_info = pe_resource_entry_info_lookup(folder_node->raw.directoryEntry->u0.Name);
	if (entry_info != NULL) {
		snprintf(dirName, sizeof(dirName), "%s/%s", g_resourceDir, entry_info->dir_name);
	} else {
		snprintf(dirName, sizeof(dirName), "%s", g_resourceDir);
	}

	if (stat(dirName, &statDir) == -1)
		mkdir(dirName, 0700);

	const pe_resource_node_t *name_node = pe_resource_find_parent_node_by_type_and_level(node, LIBPE_RDT_DIRECTORY_ENTRY, LIBPE_RDT_LEVEL2); // dirLevel == 2
	if (name_node == NULL) {
		// TODO: Should we report something?
		fprintf(stderr, "pe_resource_find_parent_node_by_type_and_level returned NULL\n");
		return;
	}
	//fprintf(stderr, "DEBUG: Name=%d\n", name_node->raw.directoryEntry->u0.Name);

	char relativeFileName[MAX_PATH]; // Wait, WHAT?!
	memset(relativeFileName, 0, sizeof(relativeFileName));

	if (namedExtract) {
		char fileName[MAX_PATH];
		memset(fileName, 0, sizeof(fileName));

		peres_build_node_filename(ctx, node, fileName, sizeof(fileName)),
		snprintf(relativeFileName, sizeof(relativeFileName), "%s/%s%s",
			dirName,
			fileName,
			entry_info != NULL ? entry_info->extension : ".bin");
	} else {
		snprintf(relativeFileName, sizeof(relativeFileName), "%s/" "%" PRIu32 "%s",
			dirName,
			name_node->raw.directoryEntry->u0.data.NameOffset,
			entry_info != NULL ? entry_info->extension : ".bin");
	}
	//printf("DEBUG: raw_data_offset=%#llx, raw_data_size=%ld, relativeFileName=%s\n", raw_data_offset, raw_data_size, relativeFileName);

	FILE *fp = fopen(relativeFileName, "wb+");
	if (fp == NULL) {
		// TODO: Should we report something?
		return;
	}
	fwrite(raw_data_ptr, raw_data_size, 1, fp);
	fclose(fp);

	output("Save On", relativeFileName);
}

static void peres_save_all_resources(pe_ctx_t *ctx, const pe_resource_node_t *node, bool namedExtract)
{
	if (node == NULL)
		return;

	if (node->type == LIBPE_RDT_DATA_ENTRY && node->dirLevel == 3) {
		peres_save_resource(ctx, node, namedExtract);
	}

	peres_save_all_resources(ctx, node->childNode, namedExtract);
	peres_save_all_resources(ctx, node->nextNode, namedExtract);
}

bool peres_contains_version_node(const pe_resource_node_t *node) {
	if (node->type != LIBPE_RDT_DIRECTORY_ENTRY)
		return false;
	if (node->dirLevel != LIBPE_RDT_LEVEL1) // dirLevel == 1 belongs to the resource type directory.
		return false;
	if (node->raw.directoryEntry->u0.data.NameOffset != RT_VERSION)
		return false;
	return true;
}

bool peres_is_version_node(const pe_resource_node_t *node) {
	return node->type == LIBPE_RDT_DATA_ENTRY;
}

static void peres_show_version(pe_ctx_t *ctx, const pe_resource_node_t *node)
{
	assert(node != NULL);

	pe_resource_node_search_result_t result_contains_version_node = {0};
	pe_resource_search_nodes(&result_contains_version_node, node, peres_contains_version_node);

	pe_resource_node_search_result_item_t *item_parent = {0};
	LL_FOREACH(result_contains_version_node.items, item_parent) {
		pe_resource_node_search_result_t result_is_version_node = {0};
		pe_resource_search_nodes(&result_is_version_node, item_parent->node, peres_is_version_node);

		pe_resource_node_search_result_item_t *item_child = {0};
		LL_FOREACH(result_is_version_node.items, item_child) {
			const uint64_t data_offset = pe_rva2ofs(ctx, item_child->node->raw.dataEntry->OffsetToData);
			const size_t data_size = item_child->node->raw.dataEntry->Size;
			const void *data_ptr = LIBPE_PTR_ADD(ctx->map_addr, 32 + data_offset); // TODO(jweyrich): The literal 32 refers to the size of the 
			if (!pe_can_read(ctx, data_ptr, data_size)) {
				LIBPE_WARNING("Cannot read VS_FIXEDFILEINFO");
				return;
			}

			const VS_FIXEDFILEINFO *info_ptr = data_ptr;
			
			char value[MAX_MSG];
			snprintf(value, MAX_MSG, "%u.%u.%u.%u",
				(uint32_t)(info_ptr->dwFileVersionMS & 0xffff0000) >> 16,
				(uint32_t)info_ptr->dwFileVersionMS & 0x0000ffff,
				(uint32_t)(info_ptr->dwFileVersionLS & 0xffff0000) >> 16,
				(uint32_t)info_ptr->dwFileVersionLS & 0x0000ffff);
			output("File Version", value);

			snprintf(value, MAX_MSG, "%u.%u.%u.%u",
				(uint32_t)(info_ptr->dwProductVersionMS & 0xffff0000) >> 16,
				(uint32_t)info_ptr->dwProductVersionMS & 0x0000ffff,
				(uint32_t)(info_ptr->dwProductVersionLS & 0xffff0000) >> 16,
				(uint32_t)info_ptr->dwProductVersionLS & 0x0000ffff);
			output("Product Version", value);
		}
		pe_resources_dealloc_node_search_result(&result_is_version_node);
	}
	pe_resources_dealloc_node_search_result(&result_contains_version_node);
}

typedef struct {
	int totalCount;
	int totalResourceDirectory;
	int totalDirectoryEntry;
	int totalDataString;
	int totalDataEntry;
} peres_stats_t;

static void peres_generate_stats(peres_stats_t *stats, const pe_resource_node_t *node) {
	if (node == NULL)
		return;
	
	stats->totalCount++;
	
	switch (node->type) {
		case LIBPE_RDT_RESOURCE_DIRECTORY:
			stats->totalResourceDirectory++;
			break;
		case LIBPE_RDT_DIRECTORY_ENTRY:
			stats->totalDirectoryEntry++;
			break;
		case LIBPE_RDT_DATA_STRING:
			stats->totalDataString++;
			break;
		case LIBPE_RDT_DATA_ENTRY:
			stats->totalDataEntry++;
			break;
	}
	
	if (node->childNode) {
		peres_generate_stats(stats, node->childNode);
	}

	if (node->nextNode) {
		peres_generate_stats(stats, node->nextNode);
	}
}

static void peres_show_stats(const pe_resource_node_t *node)
{
	peres_stats_t stats;
	memset(&stats, 0, sizeof(stats));

	peres_generate_stats(&stats, node);

	char value[MAX_MSG];

	snprintf(value, MAX_MSG, "%d", stats.totalCount);
	output("Total Structs", value);

	snprintf(value, MAX_MSG, "%d", stats.totalResourceDirectory);
	output("Total Resource Directory", value);

	snprintf(value, MAX_MSG, "%d", stats.totalDirectoryEntry);
	output("Total Directory Entry", value);

	snprintf(value, MAX_MSG, "%d", stats.totalDataString);
	output("Total Data String", value);

	snprintf(value, MAX_MSG, "%d", stats.totalDataEntry);
	output("Total Data Entry", value);
}

int main(int argc, char **argv)
{
	pev_config_t config;
	PEV_INITIALIZE(&config);

	if (argc < 3) {
		usage();
		exit(EXIT_FAILURE);
	}

	output_set_cmdline(argc, argv);

	options_t *options = parse_options(argc, argv); // opcoes

	const char *path = argv[argc-1];
	pe_ctx_t ctx;

	pe_err_e err = pe_load_file(&ctx, path);
	if (err != LIBPE_E_OK) {
		pe_error_print(stderr, err);
		return EXIT_FAILURE;
	}

	err = pe_parse(&ctx);
	if (err != LIBPE_E_OK) {
		pe_error_print(stderr, err);
		return EXIT_FAILURE;
	}

	if (!pe_is_pe(&ctx))
		EXIT_ERROR("not a valid PE file");

	output_open_document();

	pe_resources_t *resources = pe_resources(&ctx);
	if (resources == NULL || resources->err != LIBPE_E_OK) {
		LIBPE_WARNING("This file has no resources");
		return EXIT_SUCCESS;
	}

	pe_resource_node_t *root_node = resources->root_node;

	if (options->all) {
		peres_show_nodes(&ctx, root_node);
		peres_show_stats(root_node);
		peres_show_list(&ctx, root_node);
		peres_save_all_resources(&ctx, root_node, options->namedExtract);
		peres_show_version(&ctx, root_node);
	} else {
		if (options->extract)
			peres_save_all_resources(&ctx, root_node, options->namedExtract);
		if (options->info)
			peres_show_nodes(&ctx, root_node);
		if (options->list)
			peres_show_list(&ctx, root_node);
		if (options->statistics)
			peres_show_stats(root_node);
		if (options->version)
			peres_show_version(&ctx, root_node);
	}

	output_close_document();

	// libera a memoria
	free_options(options);

	// free
	err = pe_unload(&ctx);
	if (err != LIBPE_E_OK) {
		pe_error_print(stderr, err);
		return EXIT_FAILURE;
	}

	PEV_FINALIZE(&config);

	return EXIT_SUCCESS;
}

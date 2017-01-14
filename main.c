#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <json-glib/json-glib.h>

typedef gchar* gstring;

#define add_try(type, key_file_get, json_add)							\
	GError* try_ ## type (GKeyFile* gkf, JsonBuilder *builder, gchar* group_name, gchar* key) { \
		GError* err = NULL;												\
		type value = key_file_get (gkf, group_name, key, &err);			\
		if (err == NULL) {												\
			json_add (builder, value);									\
		}																\
		return err;														\
	}

add_try(gint64, g_key_file_get_int64, json_builder_add_int_value)
add_try(gdouble, g_key_file_get_double, json_builder_add_double_value)
add_try(gboolean, g_key_file_get_boolean, json_builder_add_boolean_value)
add_try(gstring, g_key_file_get_string, json_builder_add_string_value)

#define add_try_list(type, key_file_get, json_add)						\
	GError* try_ ## type ## _list (GKeyFile* gkf, JsonBuilder *builder, gchar* group_name, gchar* key) { \
		GError* err = NULL;												\
		gsize length = 0;													\
		type* list = key_file_get (gkf, group_name, key, &length, &err); \
		if (err == NULL) {												\
			if (length <= 1) {											\
				g_set_error(&err, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE, "string list has <1 element."); \
			} else {													\
				json_builder_begin_array (builder);						\
				for ( ; *list; list++ ) {								\
					json_add (builder, *list);							\
				}														\
				json_builder_end_array (builder);						\
			}															\
		}																\
		return err;														\
	}

add_try_list(gint64, g_key_file_get_integer_list, json_builder_add_int_value)
add_try_list(gdouble, g_key_file_get_double_list, json_builder_add_double_value)
add_try_list(gboolean, g_key_file_get_boolean_list, json_builder_add_boolean_value)
add_try_list(gstring, g_key_file_get_string_list, json_builder_add_string_value)

#define try_block_string_list(try)										\
	err = try (gkf, builder, *groups, *keys);							\
	if (err == NULL) {													\
		continue;														\
	} else if (err->code != G_KEY_FILE_ERROR_INVALID_VALUE) {			\
		fprintf (stderr, #try ": %s\n", err->message);					\
		g_error_free (err);												\
		return EXIT_FAILURE;											\
	}																	\
	err = NULL;

#define try_block(try)													\
	err = try (gkf, builder, *groups, *keys);							\
	if (err == NULL) {													\
		continue;														\
	} else if (err->code != G_KEY_FILE_ERROR_INVALID_VALUE) {			\
		fprintf (stderr, #try ": %s\n", err->message);					\
		g_error_free (err);												\
		return EXIT_FAILURE;											\
	}																	\
	err = NULL;

int main (int argc, char** argv) {
	if (argc < 2) {
		fprintf (stderr, "Need input file.\n");
		return EXIT_FAILURE;
	}

	#define filename argv[1]

	GKeyFile* gkf = g_key_file_new ();

	if (!g_key_file_load_from_file (gkf, filename, G_KEY_FILE_NONE, NULL)) {
		fprintf (stderr, "Could not read config file %s\n", filename);
		return EXIT_FAILURE;
	}

	JsonBuilder *builder = json_builder_new ();
	json_builder_begin_object (builder);

	gchar** groups = g_key_file_get_groups (gkf, NULL);

	GError* err = NULL;

	gchar** keys;

	for ( ; *groups; groups++ ) {
		json_builder_set_member_name (builder, *groups);

		json_builder_begin_object (builder);

		keys = g_key_file_get_keys (gkf, *groups, NULL, &err);

		if (err) {
			fprintf (stderr, "g_key_file_get_keys: %s\n", err->message);
			g_error_free(err);
			return EXIT_FAILURE;
		}

		for ( ; *keys; keys++ ) {
			json_builder_set_member_name (builder, *keys);

			/* try_block(try_gint64_list); */
			try_block(try_gdouble_list);
			try_block(try_gboolean_list);
			try_block(try_gstring_list);
			/* try_block(try_gint64); */
			try_block(try_gdouble);
			try_block(try_gboolean);
			try_block(try_gstring);
		}

		json_builder_end_object (builder);
	}

	g_key_file_free (gkf);

	json_builder_end_object (builder);

	JsonGenerator *gen = json_generator_new ();
	JsonNode* root = json_builder_get_root (builder);
	json_generator_set_root (gen, root);

	fprintf(stdout, "%s\n", json_generator_to_data (gen, NULL));

	json_node_free (root);
	g_object_unref (gen);
	g_object_unref (builder);

	return EXIT_SUCCESS;
}

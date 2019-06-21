#include "config.h"
#include <common/utils.h>
#include <common/json.c>
#include <wire/wire.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for fromwire_fail */
const void *fromwire_fail(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_fail called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

struct json {
	jsmntok_t *toks;
	char *buffer;
};

static void convert_quotes(char *first)
{
	while (*first != '\0') {
		if (*first == '\'')
			*first = '"';
		first++;
	}
}

static struct json *json_parse(const tal_t * ctx, const char *str)
{
	struct json *j = tal(ctx, struct json);
	j->buffer = tal_strdup(j, str);
	convert_quotes(j->buffer);
	bool ok;
	j->toks = json_parse_input(j, j->buffer, strlen(j->buffer), &ok);
	assert(ok);
	j->toks = json_tok_copy(j, j->toks);
	return j;
}

static void test_toks(const struct json *j, ...)
{
	const jsmntok_t * tok = j->toks + 1;
	const char *value;
	va_list(ap);
	va_start(ap, j);
	while ((value = va_arg(ap, char *)) != NULL) {
		assert(json_tok_streq(j->buffer, tok, value));
		tok++;
	}
}

static void sanity(void)
{
	struct json *j = json_parse(tmpctx, "[]");
	json_tok_remove(&j->toks, j->toks, 0);
	assert(j);
}

static void remove_one(void)
{
	struct json *j = json_parse(tmpctx, "['invoice']");
	json_tok_remove(&j->toks, j->toks + 1, 1);
	assert(j);
}

static void remove_first(void)
{
	struct json *j = json_parse(tmpctx, "['one', 'two', 'three']");
	assert(j);
	json_tok_remove(&j->toks, j->toks + 1, 1);
	assert(j->toks);

	test_toks(j, "two", "three", NULL);
	assert(tal_count(j->toks) == 3);

	j = json_parse(tmpctx, "{'1':'one', '2':'two', '3':'three'}");
	assert(j);

	json_tok_remove(&j->toks, j->toks + 1, 1);
	assert(j);
	test_toks(j, "2", "two", "3", "three", NULL);
	assert(tal_count(j->toks) == 5);

	j = json_parse(tmpctx, "{'1':'one', '2':'two', '3':'three'}");
	json_tok_remove(&j->toks, j->toks + 1, 1);
	assert(j);
	test_toks(j, "2", "two", "3", "three",  NULL);
	assert(tal_count(j->toks) == 5);

}

static void remove_last(void)
{
	struct json *j = json_parse(tmpctx, "['one', 'two', 'three']");
	json_tok_remove(&j->toks, j->toks + 3, 1);
	test_toks(j, "one", "two", NULL);
	assert(tal_count(j->toks) == 3);

	j = json_parse(tmpctx, "{'1':'one', '2':'two', '3':'three'}");
	json_tok_remove(&j->toks, j->toks + 5, 1);
	assert(j);
	test_toks(j, "1", "one", "2", "two", NULL);
	assert(tal_count(j->toks) == 5);
}

static void remove_multiple(void)
{
	struct json *j = json_parse(tmpctx, "['a', 'b', 'c', 'd', 'e']");
	json_tok_remove(&j->toks, j->toks + 1, 2);
	test_toks(j, "c", "d", "e", NULL);

	j = json_parse(tmpctx, "['a', 'b', 'c', 'd', 'e']");
	json_tok_remove(&j->toks, j->toks + 2, 2);
	test_toks(j, "a", "d", "e", NULL);

	j = json_parse(tmpctx, "{'1':'one', '2':'two', '3':'three', '4':'four'}");
	json_tok_remove(&j->toks, j->toks + 3, 2);
	assert(j);
	test_toks(j, "1", "one", "4", "four", NULL);
	assert(tal_count(j->toks) == 5);
}

static void remove_all(void)
{
	struct json *j = json_parse(tmpctx, "['a', 'b', 'c', 'd', 'e']");
	json_tok_remove(&j->toks, j->toks + 1, 5);
	assert(tal_count(j->toks) == 1);

	j = json_parse(tmpctx, "{'1':'one', '2':'two', '3':'three', '4':'four'}");
	json_tok_remove(&j->toks, j->toks + 1, 4);
	assert(tal_count(j->toks) == 1);
}

static void remove_complex(void)
{
	struct json *j = json_parse(tmpctx,
	"{'1':'one', '2':'two',"
		"'3': { '3.1': 'a', '3.2':'b', '3.3':'c' }, "
		"'4': { '4.1': 'a', '4.2':'b', '4.3':'c' }, "
	"'5':'five'}");

	json_tok_remove(&j->toks, j->toks + 5, 2);
	test_toks(j, "1", "one", "2", "two", "5", "five", NULL);
}

int main(void)
{
	setup_locale();
	setup_tmpctx();

	sanity();
	remove_one();
	remove_first();
	remove_last();
	remove_multiple();
	remove_all();
	remove_complex();

	tal_free(tmpctx);
	printf("run-json_remove ok\n");
}

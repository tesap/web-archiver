
#include <assert.h>
#include <string.h>

#include "links_replacer.h"

void printf_consume(FILE* fout, const char** ptr, const char* until) {
    assert(until > *ptr);
    int size = until - *ptr;
    fprintf(fout, "%.*s", size, *ptr);
    (*ptr) += size;
}


void replace_links(struct vec data, struct vec page_hint, FILE* fout) {
    const char* ptr = data.ptr;
    int left = data.size;
    while(left > 0) {
        left = data.size - (ptr - data.ptr);
        if (*ptr != '<') {
            printf_consume(fout, &ptr, ptr + 1);
            continue;
        }

        struct HtmlTag t = parse_html_tag(ptr);
        if (t.link.ptr == NULL) {
            printf_consume(fout, &ptr, ptr + 1);
            continue;
        }

        printf_consume(fout, &ptr, t.link.ptr);

        // fprintf(stderr, ANSI_COLOR_GREEN "  F %s\n" ANSI_COLOR_RESET, page_hint);
        // fprintf(stderr, ANSI_COLOR_YELLOW "  ? %.*s\n" ANSI_COLOR_RESET, t.link.size, t.link.ptr);

        if (strncmp(t.link.ptr, "../", 3) == 0) {
            // fprintf(stderr, ANSI_COLOR_YELLOW "S-> %.*s\n" ANSI_COLOR_RESET, t.link.size, t.link.ptr);
            printf_consume(fout, &ptr, t.tag_end);
            ptr = t.tag_end;
            continue;
        }

        /* ---- link_repr_path ---
         * Build representation of href link.
         */
        char _mem1[MAX_URL_LENGTH];
        struct vec url1 = {_mem1, 0};
        link_to_full_url(t.link, page_hint, &url1);

        char _mem2[MAX_URL_LENGTH];
        struct vec url2 = {_mem2, 0};
        only_host_path(url1, &url2);

        /* We do not want to add anything additional to representation link. */
        // LinkType lt = tag_link_type(t);
        // if (is_url_represents_file(t->link, lt)) {
        //     vec_append(out, false, vec_wrap("/index.html"));
        // }
        /* -------------------- */

        char _mem3[MAX_URL_LENGTH];
        struct vec relpath_ = {_mem3, 0};
        relpath(
            // Cut filename
            (struct vec){ page_hint.ptr, dirname_len(page_hint) },
            url2,
            &relpath_
        );

        fprintf(fout, "%.*s", relpath_.size, relpath_.ptr);
        fprintf(stderr, ANSI_COLOR_GREEN "2-> %.*s\n" ANSI_COLOR_RESET, relpath_.size, relpath_.ptr);

        ptr = t.link.ptr + t.link.size;
    }
}


/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* Simple XML manager written by Xen (@lordofxen) for managing XMP data of image formats. */

#include <SDL3/SDL.h>

#define MAX_XML_CONTENT_LENGTH 32 * 1024 * 1024 // 32 MB

static char* escape(const char* str) {
    if (!str) {
        return NULL;
    }

    size_t new_len = 0;
    const char* p = str;
    while (*p != '\0') {
        switch (*p) {
            case '<':
            case '>':
                new_len += 4;
                break;
            case '&':
                new_len += 5;
                break;
            case '\'':
            case '"':
                new_len += 6;
                break;
            default:
                new_len++;
                break;
        }
        p++;
    }

    if (new_len > SDL_SIZE_MAX - 1) {
        return NULL;
    }

    char* escaped_str = (char*)SDL_malloc(new_len + 1);
    if (!escaped_str) {
        return NULL;
    }

    size_t j = 0;
    p = str;
    while (*p != '\0') {
        switch (*p) {
            case '<':
                SDL_memcpy(&escaped_str[j], "&lt;", 4);
                j += 4;
                break;
            case '>':
                SDL_memcpy(&escaped_str[j], "&gt;", 4);
                j += 4;
                break;
            case '&':
                SDL_memcpy(&escaped_str[j], "&amp;", 5);
                j += 5;
                break;
            case '\'':
                SDL_memcpy(&escaped_str[j], "&apos;", 6);
                j += 6;
                break;
            case '"':
                SDL_memcpy(&escaped_str[j], "&quot;", 6);
                j += 6;
                break;
            default:
                escaped_str[j++] = *p;
                break;
        }
        p++;
    }
    escaped_str[j] = '\0';
    return escaped_str;
}

static size_t unescape_inplace(char* str, size_t len) {
    if (!str || len == 0) {
        return 0;
    }

    size_t i = 0, j = 0;
    while (i < len && str[i] != '\0') {
        if (str[i] == '&' && i + 3 < len) {
            if (SDL_strncmp(&str[i], "&lt;", 4) == 0) {
                str[j++] = '<';
                i += 4;
            } else if (SDL_strncmp(&str[i], "&gt;", 4) == 0) {
                str[j++] = '>';
                i += 4;
            } else if (i + 4 < len && SDL_strncmp(&str[i], "&amp;", 5) == 0) {
                str[j++] = '&';
                i += 5;
            } else if (i + 5 < len && SDL_strncmp(&str[i], "&apos;", 6) == 0) {
                str[j++] = '\'';
                i += 6;
            } else if (i + 5 < len && SDL_strncmp(&str[i], "&quot;", 6) == 0) {
                str[j++] = '"';
                i += 6;
            } else {
                str[j++] = str[i++];
            }
        } else {
            str[j++] = str[i++];
        }
    }

    if (j >= len) {
        j = len - 1;
    }
    str[j] = '\0';

    return j;
}

static const char* find_char_in_bounds(const char* str, const char* end, char c) {
    while (str < end) {
        if (*str == c) {
            return str;
        }
        str++;
    }
    return NULL;
}

static int memcasecmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        unsigned char c1 = (unsigned char)s1[i];
        unsigned char c2 = (unsigned char)s2[i];
        if (SDL_tolower(c1) != SDL_tolower(c2)) {
            return (int)SDL_tolower(c1) - (int)SDL_tolower(c2);
        }
    }
    return 0;
}

static const char* find_substr_in_bounds(const char* haystack, const char* haystack_end, 
                                       const char* needle, size_t needle_len, int case_sensitive) {
    if (!haystack || !needle || needle_len == 0 || !haystack_end) {
        return NULL;
    }
    if (haystack_end <= haystack || (size_t)(haystack_end - haystack) < needle_len) {
        return NULL;
    }

    const char* end_search = haystack_end - needle_len + 1;
    for (const char* p = haystack; p < end_search; p++) {
        if (case_sensitive ? (SDL_memcmp(p, needle, needle_len) == 0) : (memcasecmp(p, needle, needle_len) == 0)) {
            return p;
        }
    }
    return NULL;
}

static const char* find_tag_content(const char* data, const char* data_end, const char* tag, 
                                  const char** content_end, int case_sensitive) {
    if (!data || !tag || !content_end) {
        return NULL;
    }

    char start_tag[256];
    char end_tag[256];

    SDL_snprintf(start_tag, sizeof(start_tag), "<%s", tag);
    SDL_snprintf(end_tag, sizeof(end_tag), "</%s>", tag);

    size_t start_tag_len = SDL_strlen(start_tag);
    size_t end_tag_len = SDL_strlen(end_tag);
    const char* tag_start = data;

    while (tag_start < data_end) {
        if ((data_end - tag_start) >= 4 && SDL_memcmp(tag_start, "<!--", 4) == 0) {
            const char* comment_end = find_substr_in_bounds(tag_start + 4, data_end, "-->", 3, 1);
            if (!comment_end) {
                return NULL;
            }
            tag_start = comment_end + 3;
            continue;
        }
        if ((data_end - tag_start) >= 9 && SDL_memcmp(tag_start, "<![CDATA[", 9) == 0) {
            const char *cdata_end = find_substr_in_bounds(tag_start + 9, data_end, "]]>", 3, 1);
            if (!cdata_end) {
                return NULL;
            }
            tag_start = cdata_end + 3;
            continue;
        }

        tag_start = find_substr_in_bounds(tag_start, data_end, start_tag, start_tag_len, case_sensitive);
        if (!tag_start) {
            return NULL;
        }

        const char* after_tag = tag_start + start_tag_len;
        if (after_tag < data_end && ((*after_tag >= 9 && *after_tag <= 13) || *after_tag == 32 || *after_tag == '>')) {
            const char* content_start = find_char_in_bounds(after_tag, data_end, '>');
            if (content_start) {
                content_start++;
                *content_end = find_substr_in_bounds(content_start, data_end, end_tag, end_tag_len, case_sensitive);
                if (*content_end) {
                    return content_start;
                }
            }
        }
        tag_start += start_tag_len;
    }
    return NULL;
}

static char* get_content_from_tag(const char* data, size_t len, const char* tag) {
    if (!data || !tag || len == 0) {
        return NULL;
    }

    const char* data_end = data + len;
    const char* content_end;
    const char* content_start = find_tag_content(data, data_end, tag, &content_end, 1);

    if (!content_start || !content_end) {
        return NULL;
    }

    const char* alt_start = find_substr_in_bounds(content_start, content_end, "<rdf:Alt>", 9, 1);
    if (alt_start && alt_start < content_end) {
        const char* li_start = alt_start;
        const char* fallback_content = NULL;
        size_t fallback_len = 0;

        while ((li_start = find_substr_in_bounds(li_start, content_end, "<rdf:li", 7, 1)) != NULL) {
            const char* li_content_start = find_char_in_bounds(li_start, content_end, '>');
            if (!li_content_start) {
                break;
            }
            li_content_start++;

            const char* li_end = find_substr_in_bounds(li_content_start, content_end, "</rdf:li>", 9, 1);
            if (!li_end) {
                break;
            }

            while (li_content_start < li_end && SDL_isspace((unsigned char)*li_content_start)) {
                li_content_start++;
            }
            while (li_content_start < li_end && SDL_isspace((unsigned char)*(li_end - 1))) {
                li_end--;
            }
            if (li_content_start >= li_end) {
                li_start = li_end;
                continue;
            }

            const char* default_lang = find_substr_in_bounds(li_start, li_content_start, 
                                                           "xml:lang=\"x-default\"", 20, 0);
            const char* en_us_lang = find_substr_in_bounds(li_start, li_content_start, 
                                                         "xml:lang=\"en-us\"", 16, 0);

            if (default_lang || en_us_lang) {
                size_t content_len = li_end - li_content_start;
                ++content_len;
                char* result = (char*)SDL_malloc(content_len);
                if (result) {
                    SDL_memcpy(result, li_content_start, content_len - 1);
                    result[content_len - 1] = '\0';
                    unescape_inplace(result, content_len);
                }
                return result;
            }

            if (!fallback_content) {
                fallback_content = li_content_start;
                fallback_len = li_end - li_content_start;
            }

            li_start = li_end;
        }

        if (fallback_content) {
            while (fallback_content < fallback_content + fallback_len && SDL_isspace((unsigned char)*fallback_content)) {
                fallback_content++;
                fallback_len--;
            }
            while (fallback_len > 0 && SDL_isspace((unsigned char)*(fallback_content + fallback_len - 1))) {
                fallback_len--;
            }

            if (fallback_len == 0) {
                return NULL;
            }

            ++fallback_len;
            char* result = (char*)SDL_malloc(fallback_len);
            if (result) {
                SDL_memcpy(result, fallback_content, fallback_len - 1);
                result[fallback_len - 1] = '\0';
                unescape_inplace(result, fallback_len);
            }
            return result;
        }
        return NULL;
    }

    const char* seq_start = find_substr_in_bounds(content_start, content_end, "<rdf:Seq>", 9, 1);
    if (seq_start && seq_start < content_end) {
        const char* li_start = find_substr_in_bounds(seq_start, content_end, "<rdf:li", 7, 1);
        if (li_start) {
            const char* li_content_start = find_char_in_bounds(li_start, content_end, '>');
            if (li_content_start) {
                li_content_start++;
                const char* li_end = find_substr_in_bounds(li_content_start, content_end, "</rdf:li>", 9, 1);
                if (li_end) {
                    while (li_content_start < li_end && SDL_isspace((unsigned char)*li_content_start)) {
                        li_content_start++;
                    }
                    while (li_content_start < li_end && SDL_isspace((unsigned char)*(li_end - 1))) {
                        li_end--;
                    }
                    if (li_content_start >= li_end) {
                        return NULL;
                    }

                    size_t content_len = (li_end - li_content_start) + 1;
                    char* result = (char*)SDL_malloc(content_len);
                    if (result) {
                        SDL_memcpy(result, li_content_start, content_len - 1);
                        result[content_len - 1] = '\0';
                        unescape_inplace(result, content_len);
                    }
                    return result;
                }
            }
        }
        return NULL;
    }

    while (content_start < content_end && SDL_isspace((unsigned char)*content_start)) {
        content_start++;
    }
    while (content_start < content_end && SDL_isspace((unsigned char)*(content_end - 1))) {
        content_end--;
    }
    if (content_start >= content_end) {
        return NULL;
    }

    size_t content_len = (content_end - content_start) + 1;
    char* result = (char*)SDL_malloc(content_len);
    if (result) {
        SDL_memcpy(result, content_start, content_len - 1);
        result[content_len - 1] = '\0';
        unescape_inplace(result, content_len);
    }
    return result;
}

static char *get_tag(const uint8_t *data, size_t len, const char *tag) {
    if (!data || len < 4 || !tag) {
        return NULL;
    }

    return get_content_from_tag((const char*)data, len, tag);
}

char* __xmlman_GetXMPDescription(const uint8_t* data, size_t len) {
    return get_tag(data, len, "dc:description");
}

char *__xmlman_GetXMPCopyright(const uint8_t *data, size_t len) {
    return get_tag(data, len, "dc:rights");
}

char *__xmlman_GetXMPTitle(const uint8_t *data, size_t len) {
    return get_tag(data, len, "dc:title");
}

char *__xmlman_GetXMPCreator(const uint8_t *data, size_t len) {
    return get_tag(data, len, "dc:creator");
}

char *__xmlman_GetXMPCreateDate(const uint8_t *data, size_t len) {
    return get_tag(data, len, "xmp:CreateDate");
}

uint8_t *__xmlman_ConstructXMPWithRDFDescription(const char *dctitle, const char *dccreator, const char *dcdescription, const char *dcrights, const char *xmpcreatedate, size_t *outlen)
{
    if (!dctitle && !dccreator && !dcdescription && !dcrights && !xmpcreatedate) {
        if (outlen) {
            *outlen = 0;
        }
        return NULL;
    }

    const char *escaped_title = dctitle ? escape(dctitle) : NULL;
    const char *escaped_creator = dccreator ? escape(dccreator) : NULL;
    const char *escaped_description = dcdescription ? escape(dcdescription) : NULL;
    const char *escaped_rights = dcrights ? escape(dcrights) : NULL;
    const char *escaped_createdate = xmpcreatedate ? escape(xmpcreatedate) : NULL;

    if (escaped_title) {
        dctitle = escaped_title;
    }

    if (escaped_creator) {
        dccreator = escaped_creator;
    }

    if (escaped_description) {
        dcdescription = escaped_description;
    }

    if (escaped_rights) {
        dcrights = escaped_rights;
    }

    if (escaped_createdate) {
        xmpcreatedate = escaped_createdate;
    }

    const char *header =
        "<?xpacket begin=\"\" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>\n"
        "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\">\n"
        "  <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n"
        "    <rdf:Description rdf:about=\"\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:xmp=\"http://ns.adobe.com/xap/1.0/\">\n";

    const char *title_prefix =
        "      <dc:title>\n"
        "        <rdf:Alt>\n"
        "          <rdf:li xml:lang=\"x-default\">";
    const char *title_suffix = "</rdf:li>\n"
                               "        </rdf:Alt>\n"
                               "      </dc:title>\n";

    const char *creator_prefix =
        "      <dc:creator>\n"
        "        <rdf:Seq>\n"
        "          <rdf:li>";
    const char *creator_suffix = "</rdf:li>\n"
                                 "        </rdf:Seq>\n"
                                 "      </dc:creator>\n";

    const char *description_prefix =
        "      <dc:description>\n"
        "        <rdf:Alt>\n"
        "          <rdf:li xml:lang=\"x-default\">";
    const char *description_suffix = "</rdf:li>\n"
                                     "        </rdf:Alt>\n"
                                     "      </dc:description>\n";

    const char *rights_prefix =
        "      <dc:rights>\n"
        "        <rdf:Alt>\n"
        "          <rdf:li xml:lang=\"x-default\">";
    const char *rights_suffix = "</rdf:li>\n"
                                "        </rdf:Alt>\n"
                                "      </dc:rights>\n";

    const char *createdate_prefix = "      <xmp:CreateDate>";
    const char *createdate_suffix = "</xmp:CreateDate>\n";

    const char *footer =
        "    </rdf:Description>\n"
        "  </rdf:RDF>\n"
        "</x:xmpmeta>\n"
        "<?xpacket end=\"w\"?>";

    size_t total_size = SDL_strnlen(header, MAX_XML_CONTENT_LENGTH) +
                        SDL_strnlen(footer, MAX_XML_CONTENT_LENGTH) + 1;

    if (dctitle) {
        total_size += SDL_strnlen(title_prefix, MAX_XML_CONTENT_LENGTH) + SDL_strnlen(dctitle, MAX_XML_CONTENT_LENGTH) + SDL_strnlen(title_suffix, MAX_XML_CONTENT_LENGTH);
    }
    if (dccreator) {
        total_size += SDL_strnlen(creator_prefix, MAX_XML_CONTENT_LENGTH) + SDL_strnlen(dccreator, MAX_XML_CONTENT_LENGTH) + SDL_strnlen(creator_suffix, MAX_XML_CONTENT_LENGTH);
    }
    if (dcdescription) {
        total_size += SDL_strnlen(description_prefix, MAX_XML_CONTENT_LENGTH) + SDL_strnlen(dcdescription, MAX_XML_CONTENT_LENGTH) + SDL_strnlen(description_suffix, MAX_XML_CONTENT_LENGTH);
    }
    if (dcrights) {
        total_size += SDL_strnlen(rights_prefix, MAX_XML_CONTENT_LENGTH) + SDL_strnlen(dcrights, MAX_XML_CONTENT_LENGTH) + SDL_strnlen(rights_suffix, MAX_XML_CONTENT_LENGTH);
    }
    if (xmpcreatedate) {
        total_size += SDL_strnlen(createdate_prefix, MAX_XML_CONTENT_LENGTH) + SDL_strnlen(xmpcreatedate, MAX_XML_CONTENT_LENGTH) + SDL_strnlen(createdate_suffix, MAX_XML_CONTENT_LENGTH);
    }

    uint8_t *buffer = (uint8_t *)SDL_malloc(total_size);
    if (!buffer) {
        if (outlen) {
            *outlen = 0;
        }

        SDL_free((void *)escaped_title);
        SDL_free((void *)escaped_creator);
        SDL_free((void *)escaped_description);
        SDL_free((void *)escaped_rights);
        SDL_free((void *)escaped_createdate);
        return NULL;
    }

    char *p = (char *)buffer;
    p += SDL_snprintf(p, total_size, "%s", header);

    if (dctitle) {
        p += SDL_snprintf(p, total_size - (p - (char *)buffer), "%s%s%s", title_prefix, dctitle, title_suffix);
    }
    if (dccreator) {
        p += SDL_snprintf(p, total_size - (p - (char *)buffer), "%s%s%s", creator_prefix, dccreator, creator_suffix);
    }
    if (dcdescription) {
        p += SDL_snprintf(p, total_size - (p - (char *)buffer), "%s%s%s", description_prefix, dcdescription, description_suffix);
    }
    if (dcrights) {
        p += SDL_snprintf(p, total_size - (p - (char *)buffer), "%s%s%s", rights_prefix, dcrights, rights_suffix);
    }
    if (xmpcreatedate) {
        p += SDL_snprintf(p, total_size - (p - (char *)buffer), "%s%s%s", createdate_prefix, xmpcreatedate, createdate_suffix);
    }

    p += SDL_snprintf(p, total_size - (p - (char *)buffer), "%s", footer);

    if (outlen) {
        *outlen = p - (char *)buffer;
    }

    SDL_free((void *)escaped_title);
    SDL_free((void *)escaped_creator);
    SDL_free((void *)escaped_description);
    SDL_free((void *)escaped_rights);
    SDL_free((void *)escaped_createdate);

    return buffer;
}

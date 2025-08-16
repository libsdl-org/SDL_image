#include <SDL3/SDL.h>

#define MAX_XML_CONTENT_LENGTH 128 * 1024 * 1024 // 128 MB

static char* xml_escape(const char* str) {
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
                new_len += 6;
                break;
            case '"':
                new_len += 6;
                break;
            default:
                new_len++;
                break;
        }
        p++;
    }

    char* escaped_str = (char*)SDL_malloc(new_len + 1);
    if (escaped_str == NULL) {
        return NULL;
    }

    size_t j = 0;
    p = str;
    while (*p != '\0') {
        const char* replacement = NULL;
        size_t replacement_len = 0;

        switch (*p) {
            case '<':
                replacement = "&lt;";
                replacement_len = 4;
                break;
            case '>':
                replacement = "&gt;";
                replacement_len = 4;
                break;
            case '&':
                replacement = "&amp;";
                replacement_len = 5;
                break;
            case '\'':
                replacement = "&apos;";
                replacement_len = 6;
                break;
            case '"':
                replacement = "&quot;";
                replacement_len = 6;
                break;
            default:
                escaped_str[j++] = *p;
                break;
        }

        if (replacement != NULL) {
            for (size_t k = 0; k < replacement_len; k++) {
                escaped_str[j + k] = replacement[k];
            }
            j += replacement_len;
        }
        p++;
    }
    escaped_str[j] = '\0';

    return escaped_str;
}

static char* xml_unescape(const char* str, size_t maxLen) {
    size_t original_len = SDL_strnlen(str, maxLen);
    char* unescaped_str = (char*)SDL_malloc(original_len + 1);
    if (unescaped_str == NULL) {
        return NULL;
    }

    size_t i = 0;
    size_t j = 0;

    while (i < original_len) {
        if (str[i] == '&') {
            if (SDL_strncmp(str + i, "&lt;", 4) == 0) {
                unescaped_str[j++] = '<';
                i += 4;
            } else if (SDL_strncmp(str + i, "&gt;", 4) == 0) {
                unescaped_str[j++] = '>';
                i += 4;
            } else if (SDL_strncmp(str + i, "&amp;", 5) == 0) {
                unescaped_str[j++] = '&';
                i += 5;
            } else if (SDL_strncmp(str + i, "&apos;", 6) == 0) {
                unescaped_str[j++] = '\'';
                i += 6;
            } else if (SDL_strncmp(str + i, "&quot;", 6) == 0) {
                unescaped_str[j++] = '"';
                i += 6;
            } else {
                unescaped_str[j++] = str[i++];
            }
        } else {
            unescaped_str[j++] = str[i++];
        }
    }
    unescaped_str[j] = '\0';

    char* final_unescaped_str = (char*)SDL_realloc(unescaped_str, j + 1);
    if (final_unescaped_str == NULL) {
        return unescaped_str;
    }

    return final_unescaped_str;
}

static const char* GetXMLContentFromTag(const uint8_t * data, size_t len, const char* tag) {
    if (!data || !tag || len == 0) {
        return NULL;
    }

    const char* xml_data = (const char*)data;
    const char* main_tag_start = NULL;
    const char* main_tag_end = NULL;
    const char* tag_search_start = xml_data;
    const char* xml_end = xml_data + len;

    while (tag_search_start < xml_end && (tag_search_start = SDL_strstr(tag_search_start, "<")) != NULL) {
        if (tag_search_start >= xml_end) {
            break;
        }
        const char* tag_name_start = tag_search_start + 1;
        while (tag_name_start < xml_end && SDL_isspace(*tag_name_start)) {
            tag_name_start++;
        }
        size_t tag_len = SDL_strnlen(tag, xml_end - tag);
        size_t compare_len = ((size_t)(xml_end - tag_name_start) < tag_len) ? (size_t)(xml_end - tag_name_start) : tag_len;
        if (SDL_strncmp(tag_name_start, tag, compare_len) == 0) {
            const char* tag_end_brace = SDL_strstr(tag_name_start, ">");
            if (tag_end_brace) {
                main_tag_start = tag_end_brace + 1;
                break;
            }
        }
        tag_search_start++;
    }
    if (!main_tag_start) {
        return NULL;
    }

    char end_tag_str[256];
    SDL_snprintf(end_tag_str, sizeof(end_tag_str), "</%s>", tag);
    main_tag_end = SDL_strstr(main_tag_start, end_tag_str);
    if (!main_tag_end) {
        return NULL;
    }

    const char* alt_start = SDL_strstr(main_tag_start, "<rdf:Alt>");
    if (alt_start && alt_start < main_tag_end) {
        const char* li_start = alt_start;
        const char* fallback_content_start = NULL;
        const char* fallback_content_end = NULL;
        while ((li_start = SDL_strstr(li_start, "<rdf:li")) != NULL && li_start < main_tag_end) {
            const char* content_start = SDL_strstr(li_start, ">");
            if (!content_start) {
                li_start++;
                continue;
            }
            content_start++;
            const char* li_end = SDL_strstr(content_start, "</rdf:li>");
            if (!li_end) {
                li_start++;
                continue;
            }
            const char* default_lang_attr = SDL_strstr(li_start, "xml:lang=\"x-default\"");
            if (default_lang_attr && default_lang_attr < content_start) {
                size_t content_len_limit = li_end - content_start;
                size_t actual_len = SDL_strnlen(content_start, content_len_limit);
                char* final_content = (char*)SDL_malloc(actual_len + 1);
                if (final_content) {
                    SDL_strlcpy(final_content, content_start, actual_len + 1);
                }
                return final_content;
            }
            const char* en_us_lang_attr = SDL_strstr(li_start, "xml:lang=\"en-us\"");
            if (en_us_lang_attr && en_us_lang_attr < content_start) {
                size_t content_len_limit = li_end - content_start;
                size_t actual_len = SDL_strnlen(content_start, content_len_limit);
                char* final_content = (char*)SDL_malloc(actual_len + 1);
                if (final_content) {
                    SDL_strlcpy(final_content, content_start, actual_len + 1);
                }
                return final_content;
            }
            if (!fallback_content_start) {
                fallback_content_start = content_start;
                fallback_content_end = li_end;
            }

            li_start = li_end;
        }

        if (fallback_content_start) {
            size_t content_len_limit = fallback_content_end - fallback_content_start;
            size_t actual_len = SDL_strnlen(fallback_content_start, content_len_limit);
            char* final_content = (char*)SDL_malloc(actual_len + 1);
            if (final_content) {
                SDL_strlcpy(final_content, fallback_content_start, actual_len + 1);
            }
            return final_content;
        }
        return NULL;
    }

    const char* seq_start = SDL_strstr(main_tag_start, "<rdf:Seq>");
    if (seq_start && seq_start < main_tag_end) {
        const char* li_start = SDL_strstr(seq_start, "<rdf:li");
        if (li_start) {
            const char* content_start = SDL_strstr(li_start, ">");
            if (content_start) {
                content_start++;
                const char* li_end = SDL_strstr(content_start, "</rdf:li>");
                if (li_end) {
                    size_t content_len_limit = li_end - content_start;
                    size_t actual_len = SDL_strnlen(content_start, content_len_limit);
                    char* final_content = (char*)SDL_malloc(actual_len + 1);
                    if (final_content) {
                        SDL_strlcpy(final_content, content_start, actual_len + 1);
                    }
                    return final_content;
                }
            }
        }
        return NULL;
    }

    size_t content_len_limit = main_tag_end - main_tag_start;
    size_t actual_len = SDL_strnlen(main_tag_start, content_len_limit);
    char* final_content = (char*)SDL_malloc(actual_len + 1);
    if (final_content) {
        SDL_strlcpy(final_content, main_tag_start, actual_len + 1);
    }
    return final_content;
}

static const char *__gettag(const uint8_t *data, size_t len, const char *tag)
{
    if (!data || len < 2)
        return NULL;

    size_t xml_data_len = len + 1;
    uint8_t *xml_data = (uint8_t *)SDL_malloc(xml_data_len);
    if (!xml_data) {
        return NULL;
    }
    SDL_memcpy(xml_data, data, len);
    xml_data[len] = '\0'; // Null-terminate the buffer

    const char *retval = GetXMLContentFromTag(xml_data, xml_data_len, tag);
    if (!retval) {
        SDL_free(xml_data);
        xml_data = NULL;
        return NULL;
    }
    char *unescaped = xml_unescape(retval, xml_data_len);
    SDL_free((void *)retval);
    SDL_free(xml_data);
    xml_data = NULL;
    return unescaped;
}

const char* __xmlman_GetXMPDescription(const uint8_t* data, size_t len)
{
    return __gettag(data, len, "dc:description");
}

const char *__xmlman_GetXMPCopyright(const uint8_t *data, size_t len)
{
    return __gettag(data, len, "dc:rights");
}

const char *__xmlman_GetXMPTitle(const uint8_t *data, size_t len)
{
    return __gettag(data, len, "dc:title");
}

const char *__xmlman_GetXMPCreator(const uint8_t *data, size_t len)
{
    return __gettag(data, len, "dc:creator");
}

const char *__xmlman_GetXMPCreateDate(const uint8_t *data, size_t len)
{
    return __gettag(data, len, "xmp:CreateDate");
}

uint8_t *__xmlman_ConstructXMPWithRDFDescription(const char *dctitle, const char *dccreator, const char *dcdescription, const char *dcrights, const char *xmpcreatedate, size_t *outlen)
{
    if (!dctitle && !dccreator && !dcdescription && !dcrights && !xmpcreatedate) {
        if (outlen) {
            *outlen = 0;
        }
        return NULL;
    }

    const char *escaped_title = dctitle ? xml_escape(dctitle) : NULL;
    const char *escaped_creator = dccreator ? xml_escape(dccreator) : NULL;
    const char *escaped_description = dcdescription ? xml_escape(dcdescription) : NULL;
    const char *escaped_rights = dcrights ? xml_escape(dcrights) : NULL;
    const char *escaped_createdate = xmpcreatedate ? xml_escape(xmpcreatedate) : NULL;
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

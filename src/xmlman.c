#include <SDL3/SDL.h>

static const char* GetXMLContentFromTag(const uint8_t * data, size_t len, const char* tag) {
    if (!data || !tag || len == 0) {
        return NULL;
    }

    const char* xml_data = (const char*)data;
    const char* main_tag_start = NULL;
    const char* main_tag_end = NULL;
    const char* tag_search_start = xml_data;

    while ((tag_search_start = SDL_strstr(tag_search_start, "<")) != NULL) {
        const char* tag_name_start = tag_search_start + 1;
        while (SDL_isspace(*tag_name_start)) {
            tag_name_start++;
        }
        if (SDL_strncmp(tag_name_start, tag, SDL_strlen(tag)) == 0) {
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
                size_t content_len = li_end - content_start;
                char* final_content = (char*)SDL_malloc(content_len + 1);
                if (final_content) {
                    SDL_strlcpy(final_content, content_start, content_len + 1);
                }
                return final_content;
            }
            const char* en_us_lang_attr = SDL_strstr(li_start, "xml:lang=\"en-us\"");
            if (en_us_lang_attr && en_us_lang_attr < content_start) {
                size_t content_len = li_end - content_start;
                char* final_content = (char*)SDL_malloc(content_len + 1);
                if (final_content) {
                    SDL_strlcpy(final_content, content_start, content_len + 1);
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
            size_t content_len = fallback_content_end - fallback_content_start;
            char* final_content = (char*)SDL_malloc(content_len + 1);
            if (final_content) {
                SDL_strlcpy(final_content, fallback_content_start, content_len + 1);
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
                    size_t content_len = li_end - content_start;
                    char* final_content = (char*)SDL_malloc(content_len + 1);
                    if (final_content) {
                        SDL_strlcpy(final_content, content_start, content_len + 1);
                    }
                    return final_content;
                }
            }
        }
        return NULL;
    }

    size_t content_len = main_tag_end - main_tag_start;
    char* final_content = (char*)SDL_malloc(content_len + 1);
    if (final_content) {
        SDL_strlcpy(final_content, main_tag_start, content_len + 1);
    }
    return final_content;
}

const char* __xmlman_GetXMPDescription(const uint8_t* data, size_t len)
{
    return GetXMLContentFromTag(data, len, "dc:description");
}

const char *__xmlman_GetXMPCopyright(const uint8_t *data, size_t len)
{
    return GetXMLContentFromTag(data, len, "dc:rights");
}

const char *__xmlman_GetXMPTitle(const uint8_t *data, size_t len)
{
    return GetXMLContentFromTag(data, len, "dc:title");
}

const char *__xmlman_GetXMPCreator(const uint8_t *data, size_t len)
{
    return GetXMLContentFromTag(data, len, "dc:creator");
}

const char *__xmlman_GetXMPCreateDate(const uint8_t *data, size_t len)
{
    return GetXMLContentFromTag(data, len, "xmp:CreateDate");
}

uint8_t *__xmlman_ConstructXMPWithRDFDescription(const char *dctitle, const char *dccreator, const char *dcdescription, const char *dcrights, const char *xmpcreatedate, size_t *outlen)
{
    if (!dctitle && !dccreator && !dcdescription && !dcrights && !xmpcreatedate) {
        if (outlen) {
            *outlen = 0;
        }
        return NULL;
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

    size_t total_size = SDL_strlen(header) +
                        SDL_strlen(footer) + 1;

    if (dctitle) {
        total_size += SDL_strlen(title_prefix) + SDL_strlen(dctitle) + SDL_strlen(title_suffix);
    }
    if (dccreator) {
        total_size += SDL_strlen(creator_prefix) + SDL_strlen(dccreator) + SDL_strlen(creator_suffix);
    }
    if (dcdescription) {
        total_size += SDL_strlen(description_prefix) + SDL_strlen(dcdescription) + SDL_strlen(description_suffix);
    }
    if (dcrights) {
        total_size += SDL_strlen(rights_prefix) + SDL_strlen(dcrights) + SDL_strlen(rights_suffix);
    }
    if (xmpcreatedate) {
        total_size += SDL_strlen(createdate_prefix) + SDL_strlen(xmpcreatedate) + SDL_strlen(createdate_suffix);
    }

    uint8_t *buffer = (uint8_t *)SDL_malloc(total_size);
    if (!buffer) {
        if (outlen) {
            *outlen = 0;
        }
        return NULL;
    }

    char *p = (char *)buffer;
    p += SDL_snprintf(p, total_size, "%s", header);

    if (dctitle) {
        p += SDL_snprintf(p, total_size - (p - (char*)buffer), "%s%s%s", title_prefix, dctitle, title_suffix);
    }
    if (dccreator) {
        p += SDL_snprintf(p, total_size - (p - (char*)buffer), "%s%s%s", creator_prefix, dccreator, creator_suffix);
    }
    if (dcdescription) {
        p += SDL_snprintf(p, total_size - (p - (char*)buffer), "%s%s%s", description_prefix, dcdescription, description_suffix);
    }
    if (dcrights) {
        p += SDL_snprintf(p, total_size - (p - (char*)buffer), "%s%s%s", rights_prefix, dcrights, rights_suffix);
    }
    if (xmpcreatedate) {
        p += SDL_snprintf(p, total_size - (p - (char*)buffer), "%s%s%s", createdate_prefix, xmpcreatedate, createdate_suffix);
    }

    p += SDL_snprintf(p, total_size - (p - (char*)buffer), "%s", footer);

    if (outlen) {
        *outlen = p - (char*)buffer;
    }

    return buffer;
}

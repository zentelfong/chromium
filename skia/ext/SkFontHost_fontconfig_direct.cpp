/* libs/graphics/ports/SkFontHost_fontconfig_direct.cpp
**
** Copyright 2009, Google Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include "SkFontHost_fontconfig_direct.h"

#include <unistd.h>
#include <fcntl.h>

#include <fontconfig/fontconfig.h>

FontConfigDirect::FontConfigDirect()
    : next_file_id_(0) {
  FcInit();
}

bool FontConfigDirect::IsMetricCompatibleReplacement(const char* font_a,
                                                     const char* font_b)
{
    // It would be nice for fontconfig to tell us whether a given suggested
    // replacement is a "strong" match (that is, an equivalent font) or
    // a "weak" match (that is, fontconfig's next-best attempt at finding a
    // substitute).  However, I played around with the fontconfig API for
    // a good few hours and could not make it reveal this information.
    //
    // So instead, we hardcode.  These are from
    // /etc/fonts/conf.d/30-metric-aliases.conf on my Ubuntu Karmic
    // system.

    // We represent the data with a table. Two names with the same
    // id are in the same class.
    struct FontEquivClass {
        char id;
        const char name[20];
    };
    static const FontEquivClass kFontEquivClasses[] = {
        { 0, "Arial" },
        { 0, "Liberation Sans" },
        { 0, "Albany" },
        { 0, "Albany Amt" },

        { 1, "Times New Roman" },
        { 1, "Liberation Serif" },
        { 1, "Thorndale" },
        { 1, "Thorndale AMT" },

        // Note that Liberation Mono doesn't much *look* like Courier New,
        // but it's reportedly metric-compatible.
        { 2, "Courier New" },
        { 2, "Liberation Mono" },
        { 2, "Cumberland" },
        { 2, "Cumberland AMT" },

        { 3, "Helvetica" },
        { 3, "Nimbus Sans L" },

        { 4, "Times" },
        { 4, "Nimbus Roman No9 L" },

        { 5, "Courier" },
        { 5, "Nimbus Mono L" },
    };
    static const size_t kClassCount =
        sizeof(kFontEquivClasses)/sizeof(kFontEquivClasses[0]);

    int class_a = -1;
    for (size_t i = 0; i < kClassCount; ++i) {
        if (strcasecmp(kFontEquivClasses[i].name, font_a) == 0) {
            class_a = kFontEquivClasses[i].id;
            break;
        }
    }
    if (class_a == -1)
        return false;

    int class_b = -1;
    for (size_t i = 0; i < kClassCount; ++i) {
        if (strcasecmp(kFontEquivClasses[i].name, font_b) == 0) {
            class_b = kFontEquivClasses[i].id;
            break;
        }
    }

    return class_a == class_b;
}

// -----------------------------------------------------------------------------
// Normally we only return exactly the font asked for. In last-resort cases,
// the request is for one of the basic font names "Sans", "Serif" or
// "Monospace". This function tells you whether a given request is for such a
// fallback.
// -----------------------------------------------------------------------------
static bool IsFallbackFontAllowed(const std::string& family)
{
    const char* family_cstr = family.c_str();
    return strcasecmp(family_cstr, "sans") == 0 ||
           strcasecmp(family_cstr, "serif") == 0 ||
           strcasecmp(family_cstr, "monospace") == 0 ||
           // This is a special case used for a layout test
           strcasecmp(family_cstr, "NonAntiAliasedSans") == 0;
}

bool FontConfigDirect::Match(std::string* result_family,
                             unsigned* result_fileid,
                             bool fileid_valid, unsigned fileid,
                             const std::string& family, bool* is_bold,
                             bool* is_italic) {
    if (family.length() > kMaxFontFamilyLength)
        return false;

    SkAutoMutexAcquire ac(mutex_);
    FcPattern* pattern = FcPatternCreate();

    if (fileid_valid) {
        const std::map<unsigned, std::string>::const_iterator
            i = fileid_to_filename_.find(fileid);
        if (i == fileid_to_filename_.end()) {
            FcPatternDestroy(pattern);
            return false;
        }

        FcPatternAddString(pattern, FC_FILE, (FcChar8*) i->second.c_str());
    }
    if (!family.empty()) {
        FcPatternAddString(pattern, FC_FAMILY, (FcChar8*) family.c_str());
    }

    FcPatternAddInteger(pattern, FC_WEIGHT,
                        is_bold && *is_bold ? FC_WEIGHT_BOLD
                                            : FC_WEIGHT_NORMAL);
    FcPatternAddInteger(pattern, FC_SLANT,
                        is_italic && *is_italic ? FC_SLANT_ITALIC
                                                : FC_SLANT_ROMAN);
    FcPatternAddBool(pattern, FC_SCALABLE, FcTrue);

    FcConfigSubstitute(NULL, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    // Font matching:
    // CSS often specifies a fallback list of families:
    //    font-family: a, b, c, serif;
    // However, fontconfig will always do its best to find *a* font when asked
    // for something so we need a way to tell if the match which it has found is
    // "good enough" for us. Otherwise, we can return NULL which gets piped up
    // and lets WebKit know to try the next CSS family name. However, fontconfig
    // configs allow substitutions (mapping "Arial -> Helvetica" etc) and we
    // wish to support that.
    //
    // Thus, if a specific family is requested we set @family_requested. Then we
    // record two strings: the family name after config processing and the
    // family name after resolving. If the two are equal, it's a good match.
    //
    // So consider the case where a user has mapped Arial to Helvetica in their
    // config.
    //    requested family: "Arial"
    //    post_config_family: "Helvetica"
    //    post_match_family: "Helvetica"
    //      -> good match
    //
    // and for a missing font:
    //    requested family: "Monaco"
    //    post_config_family: "Monaco"
    //    post_match_family: "Times New Roman"
    //      -> BAD match
    //
    // However, we special-case fallback fonts; see IsFallbackFontAllowed().
    FcChar8* post_config_family;
    FcPatternGetString(pattern, FC_FAMILY, 0, &post_config_family);

    FcResult result;
    FcFontSet* font_set = FcFontSort(0, pattern, 0, 0, &result);
    if (!font_set) {
        FcPatternDestroy(pattern);
        return false;
    }

    // Older versions of fontconfig have a bug where they cannot select
    // only scalable fonts so we have to manually filter the results.
    FcPattern* match = NULL;
    for (int i = 0; i < font_set->nfont; ++i) {
      FcPattern* current = font_set->fonts[i];
      FcBool is_scalable;

      if (FcPatternGetBool(current, FC_SCALABLE, 0,
                           &is_scalable) != FcResultMatch ||
          !is_scalable) {
        continue;
      }

      // fontconfig can also return fonts which are unreadable
      FcChar8* c_filename;
      if (FcPatternGetString(current, FC_FILE, 0, &c_filename) != FcResultMatch)
        continue;

      if (access(reinterpret_cast<char*>(c_filename), R_OK) != 0)
        continue;

      match = current;
      break;
    }

    if (!match) {
      FcPatternDestroy(pattern);
      FcFontSetDestroy(font_set);
      return false;
    }

    if (!IsFallbackFontAllowed(family)) {
      bool acceptable_substitute = false;
      for (int id = 0; id < 255; ++id) {
        FcChar8* post_match_family;
        if (FcPatternGetString(match, FC_FAMILY, id, &post_match_family) !=
            FcResultMatch)
          break;
        acceptable_substitute =
          family.empty() ?
          true :
          (strcasecmp((char *)post_config_family,
                      (char *)post_match_family) == 0 ||
           // Workaround for Issue 12530:
           //   requested family: "Bitstream Vera Sans"
           //   post_config_family: "Arial"
           //   post_match_family: "Bitstream Vera Sans"
           // -> We should treat this case as a good match.
           strcasecmp(family.c_str(),
                      (char *)post_match_family) == 0) ||
           IsMetricCompatibleReplacement(family.c_str(),
                                         (char*)post_match_family);
        if (acceptable_substitute)
          break;
      }
      if (!acceptable_substitute) {
        FcPatternDestroy(pattern);
        FcFontSetDestroy(font_set);
        return false;
      }
    }

    FcPatternDestroy(pattern);

    FcChar8* c_filename;
    if (FcPatternGetString(match, FC_FILE, 0, &c_filename) != FcResultMatch) {
        FcFontSetDestroy(font_set);
        return false;
    }
    const std::string filename((char *) c_filename);

    unsigned out_fileid;
    if (fileid_valid) {
        out_fileid = fileid;
    } else {
        const std::map<std::string, unsigned>::const_iterator
            i = filename_to_fileid_.find(filename);
        if (i == filename_to_fileid_.end()) {
            out_fileid = next_file_id_++;
            filename_to_fileid_[filename] = out_fileid;
            fileid_to_filename_[out_fileid] = filename;
        } else {
            out_fileid = i->second;
        }
    }

    if (result_fileid)
        *result_fileid = out_fileid;

    FcChar8* c_family;
    if (FcPatternGetString(match, FC_FAMILY, 0, &c_family)) {
        FcFontSetDestroy(font_set);
        return false;
    }

    int resulting_bold;
    if (FcPatternGetInteger(match, FC_WEIGHT, 0, &resulting_bold))
      resulting_bold = FC_WEIGHT_NORMAL;

    int resulting_italic;
    if (FcPatternGetInteger(match, FC_SLANT, 0, &resulting_italic))
      resulting_italic = FC_SLANT_ROMAN;

    // If we ask for an italic font, fontconfig might take a roman font and set
    // the undocumented property FC_MATRIX to a skew matrix. It'll then say
    // that the font is italic or oblique. So, if we see a matrix, we don't
    // believe that it's italic.
    FcValue matrix;
    const bool have_matrix = FcPatternGet(match, FC_MATRIX, 0, &matrix) == 0;

    // If we ask for an italic font, fontconfig might take a roman font and set
    // FC_EMBOLDEN.
    FcValue embolden;
    const bool have_embolden =
        FcPatternGet(match, FC_EMBOLDEN, 0, &embolden) == 0;

    if (is_bold)
      *is_bold = resulting_bold > FC_WEIGHT_MEDIUM && !have_embolden;
    if (is_italic)
      *is_italic = resulting_italic > FC_SLANT_ROMAN && !have_matrix;

    if (result_family)
        *result_family = (char *) c_family;

    FcFontSetDestroy(font_set);

    return true;
}

int FontConfigDirect::Open(unsigned fileid) {
    SkAutoMutexAcquire ac(mutex_);
    const std::map<unsigned, std::string>::const_iterator
        i = fileid_to_filename_.find(fileid);
    if (i == fileid_to_filename_.end())
        return -1;

    return open(i->second.c_str(), O_RDONLY);
}

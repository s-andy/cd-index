/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#ifndef _CD_AUDIO_H_
#define _CD_AUDIO_H_

#include "data.h"

//#define INCLUDE_DURATION

#define CD_MUSIC_EXT        ".cda"

#define CD_MUSIC_MARK       "CDA"
#define CD_MUSIC_MARK_LEN   3
#define CD_MUSIC_VERSION    0x01

enum genres {
    BLUES            = 0x00, CLASSIC_ROCK     = 0x01, COUNTRY          = 0x02, DANCE             = 0x03,
    DISCO            = 0x04, FUNK             = 0x05, GRUNGE           = 0x06, HIP_HOP           = 0x07,
    JAZZ             = 0x08, METAL            = 0x09, NEW_AGE          = 0x0A, OLDIES            = 0x0B,
    OTHER            = 0x0C, POP              = 0x0D, RnB              = 0x0E, RAP               = 0x0F,
    REGGAE           = 0x10, ROCK             = 0x11, TECHNO           = 0x12, INDUSTRIAL        = 0x13,
    ALTERNATIVE      = 0x14, SKA              = 0x15, DEATH_METAL      = 0x16, PRANKS            = 0x17,
    SOUNDTRACK       = 0x18, EURO_TECHNO      = 0x19, AMBIENT          = 0x1A, TRIP_HOP          = 0x1B,
    VOCAL            = 0x1C, JAZZ_FUNK        = 0x1D, FUSION           = 0x1E, TRANCE            = 0x1F,
    CLASSICAL        = 0x20, INSTRUMENTAL     = 0x21, ACID             = 0x22, HOUSE             = 0x23,
    GAME             = 0x24, SOUND_CLIP       = 0x25, GOSPEL           = 0x26, NOISE             = 0x27,
    ALTERNROCK       = 0x28, BASS             = 0x29, SOUL             = 0x2A, PUNK              = 0x2B,
    SPACE            = 0x2C, MEDITATIVE       = 0x2D, INSTRUMENTAL_POP = 0x2E, INSTRUMENTAL_ROCK = 0x2F,
    ETHNIC           = 0x30, GOTHIC           = 0x31, DARKWAVE         = 0x32, TECHNO_INDUSTRIAL = 0x33,
    ELECTRONIC       = 0x34, POP_FOLK         = 0x35, EURODANCE        = 0x36, DREAM             = 0x37,
    SOUTHERN_ROCK    = 0x38, COMEDY           = 0x39, CULT             = 0x3A, GANGSTA           = 0x3B,
    TOP_40           = 0x3C, CHRISTIAN_RAP    = 0x3D, POPFUNK          = 0x3E, JUNGLE            = 0x3F,
    NATIVE_AMERICAN  = 0x40, CABARET          = 0x41, NEW_WAVE         = 0x42, PSYCHADELIC       = 0x43,
    RAVE             = 0x44, SHOWTUNES        = 0x45, TRAILER          = 0x46, LO_FI             = 0x47,
    TRIBAL           = 0x48, ACID_PUNK        = 0x49, ACID_JAZZ        = 0x4A, POLKA             = 0x4B,
    RETRO            = 0x4C, MUSICAL          = 0x4D, ROCK_n_ROLL      = 0x4E, HARD_ROCK         = 0x4F,
    FOLK             = 0x50, FOLK_ROCK        = 0x51, NATIONAL_FOLK    = 0x52, SWING             = 0x53,
    FAST_FUSION      = 0x54, BEBOB            = 0x55, LATIN            = 0x56, REVIVAL           = 0x57,
    CELTIC           = 0x58, BLUEGRASS        = 0x59, AVANTGARDE       = 0x5A, GOTHIC_ROCK       = 0x5B,
    PROGRESSIVE_ROCK = 0x5C, PSYCHEDELIC_ROCK = 0x5D, SYMPHONIC_ROCK   = 0x5E, SLOW_ROCK         = 0x5F,
    BIG_BAND         = 0x60, CHORUS           = 0x61, EASY_LISTENING   = 0x62, ACOUSTIC          = 0x63,
    HUMOUR           = 0x64, SPEECH           = 0x65, CHANSON          = 0x66, OPERA             = 0x67,
    CHAMBER_MUSIC    = 0x68, SONATA           = 0x69, SYMPHONY         = 0x6A, BOOTY_BASS        = 0x6B,
    PRIMUS           = 0x6C, PORN_GROOVE      = 0x6D, SATIRE           = 0x6E, SLOW_JAM          = 0x6F,
    CLUB             = 0x70, TANGO            = 0x71, SAMBA            = 0x72, FOLKLORE          = 0x73,
    BALLAD           = 0x74, POWER_BALLAD     = 0x75, RHYTHMIC_SOUL    = 0x76, FREESTYLE         = 0x77,
    DUET             = 0x78, PUNK_ROCK        = 0x79, DRUM_SOLO        = 0x7A, A_CAPELLA         = 0x7B,
    EURO_HOUSE       = 0x7C, DANCE_HALL       = 0x7D, HEAVY_METAL      = 0x7E, BLACK_METAL       = 0x7F,
    NONE             = 0xFF
};

enum langs {
    LANG_XX = 0x00, LANG_AA = 0x01, LANG_AB = 0x02, LANG_AE = 0x03, LANG_AF = 0x04, LANG_AK = 0x05,
    LANG_AM = 0x06, LANG_AN = 0x07, LANG_AR = 0x08, LANG_AS = 0x09, LANG_AV = 0x0A, LANG_AY = 0x0B,
    LANG_AZ = 0x0C, LANG_BA = 0x0D, LANG_BE = 0x0E, LANG_BG = 0x0F, LANG_BH = 0x10, LANG_BI = 0x11,
    LANG_BM = 0x12, LANG_BN = 0x13, LANG_BO = 0x14, LANG_BR = 0x15, LANG_BS = 0x16, LANG_CA = 0x17,
    LANG_CE = 0x18, LANG_CH = 0x19, LANG_CO = 0x1A, LANG_CR = 0x1B, LANG_CS = 0x1C, LANG_CU = 0x1D,
    LANG_CV = 0x1E, LANG_CY = 0x1F, LANG_DA = 0x20, LANG_DE = 0x21, LANG_DV = 0x22, LANG_DZ = 0x23,
    LANG_EE = 0x24, LANG_EL = 0x25, LANG_EN = 0x26, LANG_EO = 0x27, LANG_ES = 0x28, LANG_ET = 0x29,
    LANG_EU = 0x2A, LANG_FA = 0x2B, LANG_FF = 0x2C, LANG_FI = 0x2D, LANG_FJ = 0x2E, LANG_FO = 0x2F,
    LANG_FR = 0x30, LANG_FY = 0x31, LANG_GA = 0x32, LANG_GD = 0x33, LANG_GL = 0x34, LANG_GN = 0x35,
    LANG_GU = 0x36, LANG_GV = 0x37, LANG_HA = 0x38, LANG_HE = 0x39, LANG_HI = 0x3A, LANG_HO = 0x3B,
    LANG_HR = 0x3C, LANG_HT = 0x3D, LANG_HU = 0x3E, LANG_HY = 0x3F, LANG_HZ = 0x40, LANG_IA = 0x41,
    LANG_ID = 0x42, LANG_IE = 0x43, LANG_IG = 0x44, LANG_II = 0x45, LANG_IK = 0x46, LANG_IO = 0x47,
    LANG_IS = 0x48, LANG_IT = 0x49, LANG_IU = 0x4A, LANG_JA = 0x4B, LANG_JV = 0x4C, LANG_KA = 0x4D,
    LANG_KG = 0x4E, LANG_KI = 0x4F, LANG_KJ = 0x50, LANG_KK = 0x51, LANG_KL = 0x52, LANG_KM = 0x53,
    LANG_KN = 0x54, LANG_KO = 0x55, LANG_KR = 0x56, LANG_KS = 0x57, LANG_KU = 0x58, LANG_KV = 0x59,
    LANG_KW = 0x5A, LANG_KY = 0x5B, LANG_LA = 0x5C, LANG_LB = 0x5D, LANG_LG = 0x5E, LANG_LI = 0x5F,
    LANG_LN = 0x60, LANG_LO = 0x61, LANG_LT = 0x62, LANG_LU = 0x63, LANG_LV = 0x64, LANG_MG = 0x65,
    LANG_MH = 0x66, LANG_MI = 0x67, LANG_MK = 0x68, LANG_ML = 0x69, LANG_MN = 0x6A, LANG_MO = 0x6B,
    LANG_MR = 0x6C, LANG_MS = 0x6D, LANG_MT = 0x6E, LANG_MY = 0x6F, LANG_NA = 0x70, LANG_NB = 0x71,
    LANG_ND = 0x72, LANG_NE = 0x73, LANG_NG = 0x74, LANG_NL = 0x75, LANG_NN = 0x76, LANG_NO = 0x77,
    LANG_NR = 0x78, LANG_NV = 0x79, LANG_NY = 0x7A, LANG_OC = 0x7B, LANG_OJ = 0x7C, LANG_OM = 0x7D,
    LANG_OR = 0x7E, LANG_OS = 0x7F, LANG_PA = 0x80, LANG_PI = 0x81, LANG_PL = 0x82, LANG_PS = 0x83,
    LANG_PT = 0x84, LANG_QU = 0x85, LANG_RM = 0x86, LANG_RN = 0x87, LANG_RO = 0x88, LANG_RU = 0x89,
    LANG_RW = 0x8A, LANG_SA = 0x8B, LANG_SC = 0x8C, LANG_SD = 0x8D, LANG_SE = 0x8E, LANG_SG = 0x8F,
    LANG_SI = 0x90, LANG_SK = 0x91, LANG_SL = 0x92, LANG_SM = 0x93, LANG_SN = 0x94, LANG_SO = 0x95,
    LANG_SQ = 0x96, LANG_SR = 0x97, LANG_SS = 0x98, LANG_ST = 0x99, LANG_SU = 0x9A, LANG_SV = 0x9B,
    LANG_SW = 0x9C, LANG_TA = 0x9D, LANG_TE = 0x9E, LANG_TG = 0x9F, LANG_TH = 0xA0, LANG_TI = 0xA1,
    LANG_TK = 0xA2, LANG_TL = 0xA3, LANG_TN = 0xA4, LANG_TO = 0xA5, LANG_TR = 0xA6, LANG_TS = 0xA7,
    LANG_TT = 0xA8, LANG_TW = 0xA9, LANG_TY = 0xAA, LANG_UG = 0xAB, LANG_UK = 0xAC, LANG_UR = 0xAD,
    LANG_UZ = 0xAE, LANG_VE = 0xAF, LANG_VI = 0xB0, LANG_VO = 0xB1, LANG_WA = 0xB2, LANG_WO = 0xB3,
    LANG_XH = 0xB4, LANG_YI = 0xB5, LANG_YO = 0xB6, LANG_ZA = 0xB7, LANG_ZH = 0xB8, LANG_ZU = 0xB9
};

typedef struct {
    const char* name;
    cd_byte code;
} cd_search_item;

static const cd_search_item cd_genre_map[] = {
    { "Blues",            BLUES             }, { "Classic Rock",      CLASSIC_ROCK      },
    { "Country",          COUNTRY           }, { "Dance",             DANCE             },
    { "Disco",            DISCO             }, { "Funk",              FUNK              },
    { "Grunge",           GRUNGE            }, { "Hip-Hop",           HIP_HOP           },
    { "Jazz",             JAZZ              }, { "Metal",             METAL             },
    { "New Age",          NEW_AGE           }, { "Oldies",            OLDIES            },
    { "Other",            OTHER             }, { "Pop",               POP               },
    { "R&B",              RnB               }, { "Rap",               RAP               },
    { "Reggae",           REGGAE            }, { "Rock",              ROCK              },
    { "Techno",           TECHNO            }, { "Industrial",        INDUSTRIAL        },
    { "Alternative",      ALTERNATIVE       }, { "Ska",               SKA               },
    { "Death Metal",      DEATH_METAL       }, { "Pranks",            PRANKS            },
    { "Soundtrack",       SOUNDTRACK        }, { "Euro-Techno",       EURO_TECHNO       },
    { "Ambient",          AMBIENT           }, { "Trip-Hop",          TRIP_HOP          },
    { "Vocal",            VOCAL             }, { "Jazz+Funk",         JAZZ_FUNK         },
    { "Fusion",           FUSION            }, { "Trance",            TRANCE            },
    { "Classical",        CLASSICAL         }, { "Instrumental",      INSTRUMENTAL      },
    { "Acid",             ACID              }, { "House",             HOUSE             },
    { "Game",             GAME              }, { "Sound Clip",        SOUND_CLIP        },
    { "Gospel",           GOSPEL            }, { "Noise",             NOISE             },
    { "AlternRock",       ALTERNROCK        }, { "Bass",              BASS              },
    { "Soul",             SOUL              }, { "Punk",              PUNK              },
    { "Space",            SPACE             }, { "Meditative",        MEDITATIVE        },
    { "Instrumental Pop", INSTRUMENTAL_POP  }, { "Instrumental Rock", INSTRUMENTAL_ROCK },
    { "Ethnic",           ETHNIC            }, { "Gothic",            GOTHIC            },
    { "Darkwave",         DARKWAVE          }, { "Techno-Industrial", TECHNO_INDUSTRIAL },
    { "Electronic",       ELECTRONIC        }, { "Pop-Folk",          POP_FOLK          },
    { "Eurodance",        EURODANCE         }, { "Dream",             DREAM             },
    { "Southern Rock",    SOUTHERN_ROCK     }, { "Comedy",            COMEDY            },
    { "Cult",             CULT              }, { "Gangsta",           GANGSTA           },
    { "Top 40",           TOP_40            }, { "Christian Rap",     CHRISTIAN_RAP     },
    { "Pop/Funk",         POPFUNK           }, { "Jungle",            JUNGLE            },
    { "Native American",  NATIVE_AMERICAN   }, { "Cabaret",           CABARET           },
    { "New Wave",         NEW_WAVE          }, { "Psychadelic",       PSYCHADELIC       },
    { "Rave",             RAVE              }, { "Showtunes",         SHOWTUNES         },
    { "Trailer",          TRAILER           }, { "Lo-Fi",             LO_FI             },
    { "Tribal",           TRIBAL            }, { "Acid Punk",         ACID_PUNK         },
    { "Acid Jazz",        ACID_JAZZ         }, { "Polka",             POLKA             },
    { "Retro",            RETRO             }, { "Musical",           MUSICAL           },
    { "Rock & Roll",      ROCK_n_ROLL       }, { "Hard Rock",         HARD_ROCK         },
    { "Folk",             FOLK              }, { "Folk-Rock",         FOLK_ROCK         },
    { "National Folk",    NATIONAL_FOLK     }, { "Swing",             SWING             },
    { "Fast Fusion",      FAST_FUSION       }, { "Bebob",             BEBOB             },
    { "Latin",            LATIN             }, { "Revival",           REVIVAL           },
    { "Celtic",           CELTIC            }, { "Bluegrass",         BLUEGRASS         },
    { "Avantgarde",       AVANTGARDE        }, { "Gothic Rock",       GOTHIC_ROCK       },
    { "Progressive Rock", PROGRESSIVE_ROCK  }, { "Psychedelic Rock",  PSYCHEDELIC_ROCK  },
    { "Symphonic Rock",   SYMPHONIC_ROCK    }, { "Slow Rock",         SLOW_ROCK         },
    { "Big Band",         BIG_BAND          }, { "Chorus",            CHORUS            },
    { "Easy Listening",   EASY_LISTENING    }, { "Acoustic",          ACOUSTIC          },
    { "Humour",           HUMOUR            }, { "Speech",            SPEECH            },
    { "Chanson",          CHANSON           }, { "Opera",             OPERA             },
    { "Chamber Music",    CHAMBER_MUSIC     }, { "Sonata",            SONATA            },
    { "Symphony",         SYMPHONY          }, { "Booty Bass",        BOOTY_BASS        },
    { "Primus",           PRIMUS            }, { "Porn Groove",       PORN_GROOVE       },
    { "Satire",           SATIRE            }, { "Slow Jam",          SLOW_JAM          },
    { "Club",             CLUB              }, { "Tango",             TANGO             },
    { "Samba",            SAMBA             }, { "Folklore",          FOLKLORE          },
    { "Ballad",           BALLAD            }, { "Power Ballad",      POWER_BALLAD      },
    { "Rhythmic Soul",    RHYTHMIC_SOUL     }, { "Freestyle",         FREESTYLE         },
    { "Duet",             DUET              }, { "Punk Rock",         PUNK_ROCK         },
    { "Drum Solo",        DRUM_SOLO         }, { "A capella",         A_CAPELLA         },
    { "Euro-House",       EURO_HOUSE        }, { "Dance Hall",        DANCE_HALL        },
    { "Heavy Metal",      HEAVY_METAL       }, { "Alt. Rock",         ALTERNROCK        },
    { "Alternative Rock", ALTERNROCK        }, { "Alternative-Rock",  ALTERNROCK        },
    { "Black Metal",      BLACK_METAL       }, { "R & B",             RnB               },
    { "Alt Rock",         ALTERNROCK        },
    { NULL, NONE }
};

static const cd_search_item cd_lang_map[] = {
    { "Afar",                LANG_AA }, { "Abkhazian",       LANG_AB }, { "Avestan",         LANG_AE },
    { "Afrikaans",           LANG_AF }, { "Akan",            LANG_AK }, { "Amharic",         LANG_AM },
    { "Aragonese",           LANG_AN }, { "Arabic",          LANG_AR }, { "Assamese",        LANG_AS },
    { "Avaric",              LANG_AV }, { "Aymara",          LANG_AY }, { "Azerbaijani",     LANG_AZ },
    { "Bashkir",             LANG_BA }, { "Belarusian",      LANG_BE }, { "Bulgarian",       LANG_BG },
    { "Bihari",              LANG_BH }, { "Bislama",         LANG_BI }, { "Bambara",         LANG_BM },
    { "Bengali",             LANG_BN }, { "Tibetan",         LANG_BO }, { "Breton",          LANG_BR },
    { "Bosnian",             LANG_BS }, { "Catalan",         LANG_CA }, { "Valencian",       LANG_CA },
    { "Chechen",             LANG_CE }, { "Chamorro",        LANG_CH }, { "Corsican",        LANG_CO },
    { "Cree",                LANG_CR }, { "Czech",           LANG_CS }, { "Church Slavic",   LANG_CU },
    { "Old Slavonic",        LANG_CU }, { "Church Slavonic", LANG_CU }, { "Old Bulgarian",   LANG_CU },
    { "Old Church Slavonic", LANG_CU }, { "Chuvash",         LANG_CV }, { "Welsh",           LANG_CY },
    { "Danish",              LANG_DA }, { "German",          LANG_DE }, { "Divehi",          LANG_DV },
    { "Dhivehi",             LANG_DV }, { "Maldivian",       LANG_DV }, { "Dzongkha",        LANG_DZ },
    { "Ewe",                 LANG_EE }, { "Greek",           LANG_EL }, { "English",         LANG_EN },
    { "Esperanto",           LANG_EO }, { "Spanish",         LANG_ES }, { "Castilian",       LANG_ES },
    { "Estonian",            LANG_ET }, { "Basque",          LANG_EU }, { "Persian",         LANG_FA },
    { "Fulah",               LANG_FF }, { "Finnish",         LANG_FI }, { "Fijian",          LANG_FJ },
    { "Faroese",             LANG_FO }, { "French",          LANG_FR }, { "Western Frisian", LANG_FY },
    { "Irish",               LANG_GA }, { "Gaelic",          LANG_GD }, { "Scottish Gaelic", LANG_GD },
    { "Galician",            LANG_GL }, { "Guarani",         LANG_GN }, { "Gujarati",        LANG_GU },
    { "Manx",                LANG_GV }, { "Hausa",           LANG_HA }, { "Hebrew",          LANG_HE },
    { "Hindi",               LANG_HI }, { "Hiri Motu",       LANG_HO }, { "Croatian",        LANG_HR },
    { "Haitian",             LANG_HT }, { "Haitian Creole",  LANG_HT }, { "Hungarian",       LANG_HU },
    { "Armenian",            LANG_HY }, { "Herero",          LANG_HZ }, { "Interlingua",     LANG_IA },
    { "Indonesian",          LANG_ID }, { "Interlingue",     LANG_IE }, { "Igbo",            LANG_IG },
    { "Sichuan Yi",          LANG_II }, { "Inupiaq",         LANG_IK }, { "Ido",             LANG_IO },
    { "Icelandic",           LANG_IS }, { "Italian",         LANG_IT }, { "Inuktitut",       LANG_IU },
    { "Japanese",            LANG_JA }, { "Javanese",        LANG_JV }, { "Georgian",        LANG_KA },
    { "Kongo",               LANG_KG }, { "Kikuyu",          LANG_KI }, { "Gikuyu",          LANG_KI },
    { "Kuanyama",            LANG_KJ }, { "Kwanyama",        LANG_KJ }, { "Kazakh",          LANG_KK },
    { "Kalaallisut",         LANG_KL }, { "Greenlandic",     LANG_KL }, { "Central Khmer",   LANG_KM },
    { "Kannada",             LANG_KN }, { "Korean",          LANG_KO }, { "Kanuri",          LANG_KR },
    { "Kashmiri",            LANG_KS }, { "Kurdish",         LANG_KU }, { "Komi",            LANG_KV },
    { "Cornish",             LANG_KW }, { "Kirghiz",         LANG_KY }, { "Kyrgyz",          LANG_KY },
    { "Latin",               LANG_LA }, { "Luxembourgish",   LANG_LB }, { "Letzeburgesch",   LANG_LB },
    { "Ganda",               LANG_LG }, { "Limburgan",       LANG_LI }, { "Limburger",       LANG_LI },
    { "Limburgish",          LANG_LI }, { "Lingala",         LANG_LN }, { "Lao",             LANG_LO },
    { "Lithuanian",          LANG_LT }, { "Luba-Katanga",    LANG_LU }, { "Latvian",         LANG_LV },
    { "Malagasy",            LANG_MG }, { "Marshallese",     LANG_MH }, { "Maori",           LANG_MI },
    { "Macedonian",          LANG_MK }, { "Malayalam",       LANG_ML }, { "Mongolian",       LANG_MN },
    { "Moldavian",           LANG_MO }, { "Marathi",         LANG_MR }, { "Malay",           LANG_MS },
    { "Maltese",             LANG_MT }, { "Burmese",         LANG_MY }, { "Nauru",           LANG_NA },
    { "Bokmål",              LANG_NB }, { "Ndebele",         LANG_ND }, { "Nepali",          LANG_NE },
    { "Ndonga",              LANG_NG }, { "Dutch",           LANG_NL }, { "Flemish",         LANG_NL },
    { "Norwegian Nynorsk",   LANG_NN }, { "Nynorsk",         LANG_NN }, { "Norwegian",       LANG_NO },
    { "Ndebele",             LANG_NR }, { "Navajo",          LANG_NV }, { "Navaho",          LANG_NV },
    { "Chichewa",            LANG_NY }, { "Chewa",           LANG_NY }, { "Nyanja",          LANG_NY },
    { "Occitan",             LANG_OC }, { "Provençal",       LANG_OC }, { "Ojibwa",          LANG_OJ },
    { "Oromo",               LANG_OM }, { "Oriya",           LANG_OR }, { "Ossetian",        LANG_OS },
    { "Ossetic",             LANG_OS }, { "Panjabi",         LANG_PA }, { "Punjabi",         LANG_PA },
    { "Pali",                LANG_PI }, { "Polish",          LANG_PL }, { "Pushto",          LANG_PS },
    { "Portuguese",          LANG_PT }, { "Quechua",         LANG_QU }, { "Romansh",         LANG_RM },
    { "Rundi",               LANG_RN }, { "Romanian",        LANG_RO }, { "Russian",         LANG_RU },
    { "Kinyarwanda",         LANG_RW }, { "Sanskrit",        LANG_SA }, { "Sardinian",       LANG_SC },
    { "Sindhi",              LANG_SD }, { "Northern Sami",   LANG_SE }, { "Sango",           LANG_SG },
    { "Sinhala",             LANG_SI }, { "Sinhalese",       LANG_SI }, { "Slovak",          LANG_SK },
    { "Slovenian",           LANG_SL }, { "Samoan",          LANG_SM }, { "Shona",           LANG_SN },
    { "Somali",              LANG_SO }, { "Albanian",        LANG_SQ }, { "Serbian",         LANG_SR },
    { "Swati",               LANG_SS }, { "Sotho",           LANG_ST }, { "Sundanese",       LANG_SU },
    { "Swedish",             LANG_SV }, { "Swahili",         LANG_SW }, { "Tamil",           LANG_TA },
    { "Telugu",              LANG_TE }, { "Tajik",           LANG_TG }, { "Thai",            LANG_TH },
    { "Tigrinya",            LANG_TI }, { "Turkmen",         LANG_TK }, { "Tagalog",         LANG_TL },
    { "Tswana",              LANG_TN }, { "Tonga",           LANG_TO }, { "Turkish",         LANG_TR },
    { "Tsonga",              LANG_TS }, { "Tatar",           LANG_TT }, { "Twi",             LANG_TW },
    { "Tahitian",            LANG_TY }, { "Uighur",          LANG_UG }, { "Uyghur",          LANG_UG },
    { "Ukrainian",           LANG_UK }, { "Urdu",            LANG_UR }, { "Uzbek",           LANG_UZ },
    { "Venda",               LANG_VE }, { "Vietnamese",      LANG_VI }, { "Volapük",         LANG_VO },
    { "Walloon",             LANG_WA }, { "Wolof",           LANG_WO }, { "Xhosa",           LANG_XH },
    { "Yiddish",             LANG_YI }, { "Yoruba",          LANG_YO }, { "Zhuang",          LANG_ZA },
    { "Chuang",              LANG_ZA }, { "Chinese",         LANG_ZH }, { "Zulu",            LANG_ZU },

    { "aar", LANG_AA }, { "abk", LANG_AB }, { "ave", LANG_AE }, { "afr", LANG_AF }, { "aka", LANG_AK },
    { "amh", LANG_AM }, { "arg", LANG_AN }, { "ara", LANG_AR }, { "asm", LANG_AS }, { "ava", LANG_AV },
    { "aym", LANG_AY }, { "aze", LANG_AZ }, { "bak", LANG_BA }, { "bel", LANG_BE }, { "bul", LANG_BG },
    { "bih", LANG_BH }, { "bis", LANG_BI }, { "bam", LANG_BM }, { "ben", LANG_BN }, { "tib", LANG_BO },
    { "bre", LANG_BR }, { "bos", LANG_BS }, { "cat", LANG_CA }, { "che", LANG_CE }, { "cha", LANG_CH },
    { "cos", LANG_CO }, { "cre", LANG_CR }, { "cze", LANG_CS }, { "chu", LANG_CU }, { "chv", LANG_CV },
    { "wel", LANG_CY }, { "dan", LANG_DA }, { "ger", LANG_DE }, { "div", LANG_DV }, { "dzo", LANG_DZ },
    { "ewe", LANG_EE }, { "gre", LANG_EL }, { "eng", LANG_EN }, { "epo", LANG_EO }, { "spa", LANG_ES },
    { "est", LANG_ET }, { "baq", LANG_EU }, { "per", LANG_FA }, { "ful", LANG_FF }, { "fin", LANG_FI },
    { "fij", LANG_FJ }, { "fao", LANG_FO }, { "fre", LANG_FR }, { "fry", LANG_FY }, { "gle", LANG_GA },
    { "gla", LANG_GD }, { "glg", LANG_GL }, { "grn", LANG_GN }, { "guj", LANG_GU }, { "glv", LANG_GV },
    { "hau", LANG_HA }, { "heb", LANG_HE }, { "hin", LANG_HI }, { "hmo", LANG_HO }, { "scr", LANG_HR },
    { "hat", LANG_HT }, { "hun", LANG_HU }, { "arm", LANG_HY }, { "her", LANG_HZ }, { "ina", LANG_IA },
    { "ind", LANG_ID }, { "ile", LANG_IE }, { "ibo", LANG_IG }, { "iii", LANG_II }, { "ipk", LANG_IK },
    { "ido", LANG_IO }, { "ice", LANG_IS }, { "ita", LANG_IT }, { "iku", LANG_IU }, { "jpn", LANG_JA },
    { "jav", LANG_JV }, { "geo", LANG_KA }, { "kon", LANG_KG }, { "kik", LANG_KI }, { "kua", LANG_KJ },
    { "kaz", LANG_KK }, { "kal", LANG_KL }, { "khm", LANG_KM }, { "kan", LANG_KN }, { "kor", LANG_KO },
    { "kau", LANG_KR }, { "kas", LANG_KS }, { "kur", LANG_KU }, { "kom", LANG_KV }, { "cor", LANG_KW },
    { "kir", LANG_KY }, { "lat", LANG_LA }, { "ltz", LANG_LB }, { "lug", LANG_LG }, { "lim", LANG_LI },
    { "lin", LANG_LN }, { "lao", LANG_LO }, { "lit", LANG_LT }, { "lub", LANG_LU }, { "lav", LANG_LV },
    { "mlg", LANG_MG }, { "mah", LANG_MH }, { "mao", LANG_MI }, { "mac", LANG_MK }, { "mal", LANG_ML },
    { "mon", LANG_MN }, { "mol", LANG_MO }, { "mar", LANG_MR }, { "may", LANG_MS }, { "mlt", LANG_MT },
    { "bur", LANG_MY }, { "nau", LANG_NA }, { "nob", LANG_NB }, { "nde", LANG_ND }, { "nep", LANG_NE },
    { "ndo", LANG_NG }, { "dut", LANG_NL }, { "nno", LANG_NN }, { "nor", LANG_NO }, { "nbl", LANG_NR },
    { "nav", LANG_NV }, { "nya", LANG_NY }, { "oci", LANG_OC }, { "oji", LANG_OJ }, { "orm", LANG_OM },
    { "ori", LANG_OR }, { "oss", LANG_OS }, { "pan", LANG_PA }, { "pli", LANG_PI }, { "pol", LANG_PL },
    { "pus", LANG_PS }, { "por", LANG_PT }, { "que", LANG_QU }, { "roh", LANG_RM }, { "run", LANG_RN },
    { "rum", LANG_RO }, { "rus", LANG_RU }, { "kin", LANG_RW }, { "san", LANG_SA }, { "srd", LANG_SC },
    { "snd", LANG_SD }, { "sme", LANG_SE }, { "sag", LANG_SG }, { "sin", LANG_SI }, { "slo", LANG_SK },
    { "slv", LANG_SL }, { "smo", LANG_SM }, { "sna", LANG_SN }, { "som", LANG_SO }, { "alb", LANG_SQ },
    { "scc", LANG_SR }, { "ssw", LANG_SS }, { "sot", LANG_ST }, { "sun", LANG_SU }, { "swe", LANG_SV },
    { "swa", LANG_SW }, { "tam", LANG_TA }, { "tel", LANG_TE }, { "tgk", LANG_TG }, { "tha", LANG_TH },
    { "tir", LANG_TI }, { "tuk", LANG_TK }, { "tgl", LANG_TL }, { "tsn", LANG_TN }, { "ton", LANG_TO },
    { "tur", LANG_TR }, { "tso", LANG_TS }, { "tat", LANG_TT }, { "twi", LANG_TW }, { "tah", LANG_TY },
    { "uig", LANG_UG }, { "ukr", LANG_UK }, { "urd", LANG_UR }, { "uzb", LANG_UZ }, { "ven", LANG_VE },
    { "vie", LANG_VI }, { "vol", LANG_VO }, { "wln", LANG_WA }, { "wol", LANG_WO }, { "xho", LANG_XH },
    { "yid", LANG_YI }, { "yor", LANG_YO }, { "zha", LANG_ZA }, { "chi", LANG_ZH }, { "zul", LANG_ZU },

    { "aa", LANG_AA }, { "ab", LANG_AB }, { "ae", LANG_AE }, { "af", LANG_AF }, { "ak", LANG_AK },
    { "am", LANG_AM }, { "an", LANG_AN }, { "ar", LANG_AR }, { "as", LANG_AS }, { "av", LANG_AV },
    { "ay", LANG_AY }, { "az", LANG_AZ }, { "ba", LANG_BA }, { "be", LANG_BE }, { "bg", LANG_BG },
    { "bh", LANG_BH }, { "bi", LANG_BI }, { "bm", LANG_BM }, { "bn", LANG_BN }, { "bo", LANG_BO },
    { "br", LANG_BR }, { "bs", LANG_BS }, { "ca", LANG_CA }, { "ce", LANG_CE }, { "ch", LANG_CH },
    { "co", LANG_CO }, { "cr", LANG_CR }, { "cs", LANG_CS }, { "cu", LANG_CU }, { "cv", LANG_CV },
    { "cy", LANG_CY }, { "da", LANG_DA }, { "de", LANG_DE }, { "dv", LANG_DV }, { "dz", LANG_DZ },
    { "ee", LANG_EE }, { "el", LANG_EL }, { "en", LANG_EN }, { "eo", LANG_EO }, { "es", LANG_ES },
    { "et", LANG_ET }, { "eu", LANG_EU }, { "fa", LANG_FA }, { "ff", LANG_FF }, { "fi", LANG_FI },
    { "fj", LANG_FJ }, { "fo", LANG_FO }, { "fr", LANG_FR }, { "fy", LANG_FY }, { "ga", LANG_GA },
    { "gd", LANG_GD }, { "gl", LANG_GL }, { "gn", LANG_GN }, { "gu", LANG_GU }, { "gv", LANG_GV },
    { "ha", LANG_HA }, { "he", LANG_HE }, { "hi", LANG_HI }, { "ho", LANG_HO }, { "hr", LANG_HR },
    { "ht", LANG_HT }, { "hu", LANG_HU }, { "hy", LANG_HY }, { "hz", LANG_HZ }, { "ia", LANG_IA },
    { "id", LANG_ID }, { "ie", LANG_IE }, { "ig", LANG_IG }, { "ii", LANG_II }, { "ik", LANG_IK },
    { "io", LANG_IO }, { "is", LANG_IS }, { "it", LANG_IT }, { "iu", LANG_IU }, { "ja", LANG_JA },
    { "jv", LANG_JV }, { "ka", LANG_KA }, { "kg", LANG_KG }, { "ki", LANG_KI }, { "kj", LANG_KJ },
    { "kk", LANG_KK }, { "kl", LANG_KL }, { "km", LANG_KM }, { "kn", LANG_KN }, { "ko", LANG_KO },
    { "kr", LANG_KR }, { "ks", LANG_KS }, { "ku", LANG_KU }, { "kv", LANG_KV }, { "kw", LANG_KW },
    { "ky", LANG_KY }, { "la", LANG_LA }, { "lb", LANG_LB }, { "lg", LANG_LG }, { "li", LANG_LI },
    { "ln", LANG_LN }, { "lo", LANG_LO }, { "lt", LANG_LT }, { "lu", LANG_LU }, { "lv", LANG_LV },
    { "mg", LANG_MG }, { "mh", LANG_MH }, { "mi", LANG_MI }, { "mk", LANG_MK }, { "ml", LANG_ML },
    { "mn", LANG_MN }, { "mo", LANG_MO }, { "mr", LANG_MR }, { "ms", LANG_MS }, { "mt", LANG_MT },
    { "my", LANG_MY }, { "na", LANG_NA }, { "nb", LANG_NB }, { "nd", LANG_ND }, { "ne", LANG_NE },
    { "ng", LANG_NG }, { "nl", LANG_NL }, { "nn", LANG_NN }, { "no", LANG_NO }, { "nr", LANG_NR },
    { "nv", LANG_NV }, { "ny", LANG_NY }, { "oc", LANG_OC }, { "oj", LANG_OJ }, { "om", LANG_OM },
    { "or", LANG_OR }, { "os", LANG_OS }, { "pa", LANG_PA }, { "pi", LANG_PI }, { "pl", LANG_PL },
    { "ps", LANG_PS }, { "pt", LANG_PT }, { "qu", LANG_QU }, { "rm", LANG_RM }, { "rn", LANG_RN },
    { "ro", LANG_RO }, { "ru", LANG_RU }, { "rw", LANG_RW }, { "sa", LANG_SA }, { "sc", LANG_SC },
    { "sd", LANG_SD }, { "se", LANG_SE }, { "sg", LANG_SG }, { "si", LANG_SI }, { "sk", LANG_SK },
    { "sl", LANG_SL }, { "sm", LANG_SM }, { "sn", LANG_SN }, { "so", LANG_SO }, { "sq", LANG_SQ },
    { "sr", LANG_SR }, { "ss", LANG_SS }, { "st", LANG_ST }, { "su", LANG_SU }, { "sv", LANG_SV },
    { "sw", LANG_SW }, { "ta", LANG_TA }, { "te", LANG_TE }, { "tg", LANG_TG }, { "th", LANG_TH },
    { "ti", LANG_TI }, { "tk", LANG_TK }, { "tl", LANG_TL }, { "tn", LANG_TN }, { "to", LANG_TO },
    { "tr", LANG_TR }, { "ts", LANG_TS }, { "tt", LANG_TT }, { "tw", LANG_TW }, { "ty", LANG_TY },
    { "ug", LANG_UG }, { "uk", LANG_UK }, { "ur", LANG_UR }, { "uz", LANG_UZ }, { "ve", LANG_VE },
    { "vi", LANG_VI }, { "vo", LANG_VO }, { "wa", LANG_WA }, { "wo", LANG_WO }, { "xh", LANG_XH },
    { "yi", LANG_YI }, { "yo", LANG_YO }, { "za", LANG_ZA }, { "zh", LANG_ZH }, { "zu", LANG_ZU },

    { NULL, LANG_XX }
};

typedef struct {
    char mark[3];           // "CDM"
    cd_byte version;        // 0x01
} packed(cd_audio_mark);

typedef struct {
    cd_offset offset;
    cd_word bitrate;
    cd_word freq;
    cd_byte mpeg:2;
    cd_byte layer:2;
    cd_byte mode:2;
    cd_byte copy:1;
    cd_byte orig:1;
    cd_byte lang;
#ifdef INCLUDE_DURATION
    cd_word seconds;
#endif
    char title[128];
    char artist[64];
    char album[96];
    cd_word year;
    cd_byte genre;
    cd_byte track;
} packed(cd_audio_entry);

#endif /* _CD_AUDIO_H_ */

/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   User interface services - Virtual key code definitions, conversion maps,
   and also locales with their Microsoft Windows locale code for the keyboard layout

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2009

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// Refer to "Windows XP/Server 2003 - List of Locale IDs, Input Locale, and Language Collection"
// http://www.microsoft.com/globaldev/reference/winxp/xp-lcid.mspx

#ifndef __LOCALES_H
#define __LOCALES_H

#define AFRIKAANS				0x0436
#define ALBANIAN				0x041c
#define ALSATIAN				0x0484
#define AMHARIC					0x045E
#define ARABIC_SAUDI_ARABIA			0x0401
#define ARABIC_IRAQ				0x0801
#define ARABIC_EGYPT				0x0c01
#define ARABIC_LIBYA				0x1001
#define ARABIC_ALGERIA				0x1401
#define ARABIC_MOROCCO				0x1801
#define ARABIC_TUNISIA				0x1c01
#define ARABIC_OMAN				0x2001
#define ARABIC_YEMEN				0x2401
#define ARABIC_SYRIA				0x2801
#define ARABIC_JORDAN				0x2c01
#define ARABIC_LEBANON				0x3001
#define ARABIC_KUWAIT				0x3401
#define ARABIC_UAE				0x3801
#define ARABIC_BAHRAIN				0x3c01
#define ARABIC_QATAR				0x4001
#define ARMENIAN				0x042b
#define ASSAMESE				0x044D
#define AZERI_LATIN				0x042c
#define AZERI_CYRILLIC				0x082c
#define BASHKIR					0x046D
#define BASQUE					0x042d
#define BELARUSIAN				0x0423
#define BENGALI_INDIA				0x0445
#define BOSNIAN_LATIN				0x141A
#define	BRETON					0x047E
#define BULGARIAN				0x0402
#define CATALAN					0x0403
#define CHINESE_TAIWAN				0x0404
#define CHINESE_PRC				0x0804
#define CHINESE_HONG_KONG			0x0c04
#define CHINESE_SINGAPORE			0x1004
#define CHINESE_MACAU				0x1404
#define CROATIAN				0x041a
#define CROATIAN_BOSNIA_HERZEGOVINA		0x101A
#define CZECH					0x0405
#define DANISH					0x0406
#define DARI					0x048C
#define DIVEHI					0x0465
#define DUTCH_STANDARD				0x0413
#define DUTCH_BELGIAN				0x0813
#define ENGLISH_UNITED_STATES			0x0409
#define ENGLISH_UNITED_KINGDOM			0x0809
#define ENGLISH_AUSTRALIAN			0x0c09
#define ENGLISH_CANADIAN			0x1009
#define ENGLISH_NEW_ZEALAND			0x1409
#define ENGLISH_INDIA				0x4009
#define ENGLISH_IRELAND				0x1809
#define ENGLISH_MALAYSIA			0x4409
#define ENGLISH_SOUTH_AFRICA			0x1c09
#define ENGLISH_JAMAICA				0x2009
#define ENGLISH_CARIBBEAN			0x2409
#define ENGLISH_BELIZE				0x2809
#define ENGLISH_TRINIDAD			0x2c09
#define ENGLISH_ZIMBABWE			0x3009
#define ENGLISH_PHILIPPINES			0x3409
#define ENGLISH_SINGAPORE			0x4809
#define ESTONIAN				0x0425
#define FAEROESE				0x0438
#define FARSI					0x0429
#define FILIPINO				0x0464
#define FINNISH					0x040b
#define FRENCH_STANDARD				0x040c
#define FRENCH_BELGIAN				0x080c
#define FRENCH_CANADIAN				0x0c0c
#define FRENCH_SWISS				0x100c
#define FRENCH_LUXEMBOURG			0x140c
#define FRENCH_MONACO				0x180c
#define FRISIAN					0x0462
#define GEORGIAN				0x0437
#define GALICIAN				0x0456
#define GERMAN_STANDARD				0x0407
#define GERMAN_SWISS				0x0807
#define GERMAN_AUSTRIAN				0x0c07
#define GERMAN_LUXEMBOURG			0x1007
#define GERMAN_LIECHTENSTEIN			0x1407
#define GREEK					0x0408
#define GREENLANDIC				0x046F
#define GUJARATI				0x0447
#define HEBREW					0x040d
#define HINDI					0x0439
#define HUNGARIAN				0x040e
#define ICELANDIC				0x040f
#define IGBO					0x0470
#define INDONESIAN				0x0421
#define IRISH					0x083C
#define ITALIAN_STANDARD			0x0410
#define ITALIAN_SWISS				0x0810
#define JAPANESE				0x0411
#define KANNADA					0x044b
#define KAZAKH					0x043f
#define KHMER					0x0453
#define KICHE					0x0486
#define KINYARWANDA				0x0487
#define KONKANI					0x0457
#define KOREAN					0x0412
#define KYRGYZ					0x0440
#define LAO					0x0454
#define LATVIAN					0x0426
#define LITHUANIAN				0x0427
#define LOWER_SORBIAN				0x082E
#define LUXEMBOURGISH				0x046E
#define MACEDONIAN				0x042f
#define MALAY_MALAYSIA				0x043e
#define MALAY_BRUNEI_DARUSSALAM			0x083e
#define MALAYALAM				0x044c
#define MALTESE					0x043a
#define MAPUDUNGUN				0x047A
#define MAORI					0x0481
#define MARATHI					0x044e
#define MOHAWK					0x047C
#define MONGOLIAN				0x0450
#define NEPALI					0x0461
#define NORWEGIAN_BOKMAL			0x0414
#define NORWEGIAN_NYNORSK			0x0814
#define OCCITAN					0x0482
#define ORIYA					0x0448
#define PASHTO					0x0463
#define POLISH					0x0415
#define PORTUGUESE_BRAZILIAN			0x0416
#define PORTUGUESE_STANDARD			0x0816
#define PUNJABI					0x0446
#define QUECHUA_BOLIVIA				0x046b
#define QUECHUA_ECUADOR				0x086b
#define QUECHUA_PERU				0x0c6b
#define ROMANIAN				0x0418
#define ROMANSH					0x0417
#define RUSSIAN					0x0419
#define SAMI_INARI				0x243b
#define SAMI_LULE_NORWAY			0x103b
#define SAMI_LULE_SWEDEN			0x143b
#define SAMI_NORTHERN_FINLAND			0x0c3b
#define SAMI_NORTHERN_NORWAY			0x043b
#define SAMI_NORTHERN_SWEDEN			0x083b
#define SAMI_SKOLT				0x203b
#define SAMI_SOUTHERN_NORWAY			0x183b
#define SAMI_SOUTHERN_SWEDEN			0x1c3b
#define SANSKRIT				0x044f
#define SERBIAN_LATIN				0x081a
#define SERBIAN_LATIN_BOSNIA_HERZEGOVINA	0x181a
#define SERBIAN_CYRILLIC			0x0c1a
#define SERBIAN_CYRILLIC_BOSNIA_HERZEGOVINA	0x1c1a
#define SESOTHO_SA_LEBOA			0x046C
#define SINHALA					0x045B
#define SLOVAK					0x041b
#define SLOVENIAN				0x0424
#define SPANISH_TRADITIONAL_SORT		0x040a
#define SPANISH_MEXICAN				0x080a
#define SPANISH_MODERN_SORT			0x0c0a
#define SPANISH_GUATEMALA			0x100a
#define SPANISH_COSTA_RICA			0x140a
#define SPANISH_PANAMA				0x180a
#define SPANISH_DOMINICAN_REPUBLIC		0x1c0a
#define SPANISH_VENEZUELA			0x200a
#define SPANISH_COLOMBIA			0x240a
#define SPANISH_PERU				0x280a
#define SPANISH_ARGENTINA			0x2c0a
#define SPANISH_ECUADOR				0x300a
#define SPANISH_CHILE				0x340a
#define SPANISH_UNITED_STATES			0x540A
#define SPANISH_URUGUAY				0x380a
#define SPANISH_PARAGUAY			0x3c0a
#define SPANISH_BOLIVIA				0x400a
#define SPANISH_EL_SALVADOR			0x440a
#define SPANISH_HONDURAS			0x480a
#define SPANISH_NICARAGUA			0x4c0a
#define SPANISH_PUERTO_RICO			0x500a
#define SWAHILI					0x0441
#define SWEDISH					0x041d
#define SWEDISH_FINLAND				0x081d
#define SYRIAC					0x045a
#define TAMIL					0x0449
#define TATAR					0x0444
#define TELUGU					0x044a
#define THAI					0x041e
#define TIBETAN_BHUTAN				0x0851
#define TIBETAN_PRC				0x0451
#define TSWANA					0x0432
#define UKRAINIAN				0x0422
#define TURKISH					0x041f
#define TURKMEN					0x0442
#define UIGHUR					0x0480
#define UPPER_SORBIAN				0x042E
#define URDU					0x0420
#define URDU_INDIA				0x0820
#define UZBEK_LATIN				0x0443
#define UZBEK_CYRILLIC				0x0843
#define VIETNAMESE				0x042a
#define WELSH					0x0452
#define WOLOF					0x0488
#define XHOSA					0x0434
#define YAKUT					0x0485
#define YI					0x0478
#define YORUBA					0x046A
#define ZULU					0x0435


typedef struct
{
	// Two or three letter language code
	char language[4];

	// Two or three letter country code (Sometimes with Cyrl_ prefix)
	char country[10];

	// 32-bit unsigned integer corresponding to the locale
	unsigned int code;

} locale;


// Refer to MSDN article "Locale Identifier Constants and Strings"
// http://msdn.microsoft.com/en-us/library/ms776260.aspx

locale locales[] =
{
	{  "af", "ZA", AFRIKAANS }, // Afrikaans (South Africa)
	{  "sq", "AL", ALBANIAN }, // Albanian (Albania)
	{ "gsw", "FR", ALSATIAN }, // Windows Vista and later: Alsatian (France)
	{  "am", "ET", AMHARIC }, // Windows Vista and later: Amharic (Ethiopia)
	{  "ar", "DZ", ARABIC_ALGERIA }, // Arabic (Algeria)
	{  "ar", "BH", ARABIC_BAHRAIN }, // Arabic (Bahrain)
	{  "ar", "EG", ARABIC_EGYPT }, // Arabic (Egypt)
	{  "ar", "IQ", ARABIC_IRAQ }, // Arabic (Iraq)
	{  "ar", "JO", ARABIC_JORDAN }, // Arabic (Jordan)
	{  "ar", "KW", ARABIC_KUWAIT }, // Arabic (Kuwait)
	{  "ar", "LB", ARABIC_LEBANON }, // Arabic (Lebanon)
	{  "ar", "LY", ARABIC_LIBYA }, // Arabic (Libya)
	{  "ar", "MA", ARABIC_MOROCCO }, // Arabic (Morocco)
	{  "ar", "OM", ARABIC_OMAN }, // Arabic (Oman)
	{  "ar", "QA", ARABIC_QATAR }, // Arabic (Qatar)
	{  "ar", "SA", ARABIC_SAUDI_ARABIA }, // Arabic (Saudi Arabia)
	{  "ar", "SY", ARABIC_SYRIA }, // Arabic (Syria)
	{  "ar", "TN", ARABIC_TUNISIA }, // Arabic (Tunisia)
	{  "ar", "AE", ARABIC_UAE }, // Arabic (U.A.E.)
	{  "ar", "YE", ARABIC_YEMEN }, // Arabic (Yemen)
	{  "az", "AZ", AZERI_LATIN }, // Azeri (Latin)
	{  "az", "Cyrl_AZ", AZERI_CYRILLIC }, // Azeri (Cyrillic)
	{  "hy", "AM", ARMENIAN }, // Windows 2000 and later: Armenian (Armenia)
	{  "as", "IN", ASSAMESE }, // Windows Vista and later: Assamese (India)
	{  "ba", "RU", BASHKIR }, // Windows Vista and later: Bashkir (Russia)
	{  "eu", "ES", BASQUE }, // Basque (Basque)
	{  "be", "BY", BELARUSIAN }, // Belarusian (Belarus)
	{  "bn", "IN", BENGALI_INDIA }, // Windows XP SP2 and later: Bengali (India)
	{  "br", "FR", BRETON }, // Breton (France)
	{  "bs", "BA", BOSNIAN_LATIN }, // Bosnian (Latin)
	{  "bg", "BG", BULGARIAN }, // Bulgarian (Bulgaria)
	{  "ca", "ES", CATALAN }, // Catalan (Catalan)
	{  "zh", "HK", CHINESE_HONG_KONG }, // Chinese (Hong Kong SAR, PRC)
	{  "zh", "MO", CHINESE_MACAU }, // Windows 98/Me, Windows XP and later: Chinese (Macao SAR)
	{  "zh", "CN", CHINESE_PRC }, // Chinese (PRC)
	{  "zh", "SG", CHINESE_SINGAPORE }, // Chinese (Singapore)
	{  "zh", "TW", CHINESE_TAIWAN }, // Chinese (Taiwan)
	{  "hr", "BA", CROATIAN_BOSNIA_HERZEGOVINA }, // Windows XP SP2 and later: Croatian (Bosnia and Herzegovina, Latin)
	{  "hr", "HR", CROATIAN }, // Croatian (Croatia)
	{  "cs", "CZ", CZECH }, // Czech (Czech Republic)
	{  "da", "DK", DANISH }, // Danish (Denmark)
	{ "prs", "AF", DARI }, // Windows XP and later: Dari (Afghanistan)
	{  "dv", "MV", DIVEHI }, // Windows XP and later: Divehi (Maldives)
	{  "nl", "BE", DUTCH_BELGIAN }, // Dutch (Belgium)
	{  "nl", "NL", DUTCH_STANDARD }, // Dutch (Netherlands)
	{  "en", "AU", ENGLISH_AUSTRALIAN }, // English (Australia)
	{  "en", "BZ", ENGLISH_BELIZE }, // English (Belize)
	{  "en", "CA", ENGLISH_CANADIAN }, // English (Canada)
	{  "en", "CB", ENGLISH_CARIBBEAN }, // English (Carribean)
	{  "en", "IN", ENGLISH_INDIA }, // Windows Vista and later: English (India)
	{  "en", "IE", ENGLISH_IRELAND }, // English (Ireland)
	{  "en", "JM", ENGLISH_JAMAICA }, // English (Jamaica)
	{  "en", "MY", ENGLISH_MALAYSIA }, // Windows Vista and later: English (Malaysia)
	{  "en", "NZ", ENGLISH_NEW_ZEALAND }, // English (New Zealand)
	{  "en", "PH", ENGLISH_PHILIPPINES }, // Windows 98/Me, Windows 2000 and later: English (Philippines)
	{  "en", "SG", ENGLISH_SINGAPORE }, // Windows Vista and later: English (Singapore)
	{  "en", "ZA", ENGLISH_SOUTH_AFRICA }, // English (South Africa)
	{  "en", "TT", ENGLISH_TRINIDAD }, // English (Trinidad and Tobago)
	{  "en", "GB", ENGLISH_UNITED_KINGDOM }, // English (United Kingdom)
	{  "en", "US", ENGLISH_UNITED_STATES }, // English (United States)
	{  "en", "ZW", ENGLISH_ZIMBABWE }, // Windows 98/Me, Windows 2000 and later: English (Zimbabwe)
	{  "et", "EE", ESTONIAN }, // Estonian (Estonia)
	{  "fo", "FO", FAEROESE }, // Faroese (Faroe Islands)
	{ "fil", "PH", FILIPINO }, // Windows XP SP2 and later (downloadable); Windows Vista and later: Filipino (Philippines)
	{  "fi", "FI", FINNISH }, // Finnish (Finland)
	{  "fr", "BE", FRENCH_BELGIAN }, // French (Belgium)
	{  "fr", "CA", FRENCH_CANADIAN }, // French (Canada)
	{  "fr", "FR", FRENCH_STANDARD }, // French (France)
	{  "fr", "LU", FRENCH_LUXEMBOURG }, // French (Luxembourg)
	{  "fr", "MC", FRENCH_MONACO }, // French (Monaco)
	{  "fr", "CH", FRENCH_SWISS }, // French (Switzerland)
	{  "fy", "NL", FRISIAN }, // Windows XP SP2 and later (downloadable); Windows Vista and later: Frisian (Netherlands)
	{  "gl", "ES", GALICIAN }, // Windows XP and later: Galician (Spain)
	{  "ka", "GE", GEORGIAN }, // Windows 2000 and later: Georgian (Georgia)
	{  "de", "AT", GERMAN_AUSTRIAN }, // German (Austria)
	{  "de", "DE", GERMAN_STANDARD }, // German (Germany)
	{  "de", "LI", GERMAN_LIECHTENSTEIN }, // German (Liechtenstein)
	{  "de", "LU", GERMAN_LUXEMBOURG }, // German (Luxembourg)
	{  "de", "CH", GERMAN_SWISS }, // German (Switzerland)
	{  "el", "GR", GREEK }, // Greek (Greece)
	{  "kl", "GL", GREENLANDIC }, // Windows Vista and later: Greenlandic (Greenland)
	{  "gu", "IN", GUJARATI }, // Windows XP and later: Gujarati (India)
	{  "he", "IL", HEBREW }, // Hebrew (Israel)
	{  "hi", "IN", HINDI }, // Windows 2000 and later: Hindi (India)
	{  "hu", "HU", HUNGARIAN }, // Hungarian (Hungary)
	{  "is", "IS", ICELANDIC }, // Icelandic (Iceland)
	{  "ig", "NG", IGBO }, // Igbo (Nigeria)
	{  "id", "ID", INDONESIAN }, // Indonesian (Indonesia)
	{  "ga", "IE", IRISH }, // Windows XP SP2 and later (downloadable); Windows Vista and later: Irish (Ireland)
	{  "it", "IT", ITALIAN_STANDARD }, // Italian (Italy)
	{  "it", "CH", ITALIAN_SWISS }, // Italian (Switzerland)
	{  "ja", "JP", JAPANESE }, // Japanese (Japan)
	{  "kn", "IN", KANNADA }, // Windows XP and later: Kannada (India)
	{  "kk", "KZ", KAZAKH }, // Windows 2000 and later: Kazakh (Kazakhstan)
	{  "kh", "KH", KHMER }, // Windows Vista and later: Khmer (Cambodia)
	{ "qut", "GT", KICHE }, // Windows Vista and later: K'iche (Guatemala)
	{  "rw", "RW", KINYARWANDA }, // Windows Vista and later: Kinyarwanda (Rwanda)
	{ "kok", "IN", KONKANI }, // Windows 2000 and later: Konkani (India)
	{  "ko", "KR", KOREAN }, // Korean (Korea)
	{  "ky", "KG", KYRGYZ }, // Windows XP and later: Kyrgyz (Kyrgyzstan)
	{  "lo", "LA", LAO }, // Windows Vista and later: Lao (Lao PDR)
	{  "lv", "LV", LATVIAN }, // Latvian (Latvia)
	{  "lt", "LT", LITHUANIAN }, // Lithuanian (Lithuania)
	{ "dsb", "DE", LOWER_SORBIAN }, // Windows Vista and later: Lower Sorbian (Germany)
	{  "lb", "LU", LUXEMBOURGISH }, // Windows XP SP2 and later (downloadable); Windows Vista and later: Luxembourgish (Luxembourg)
	{  "mk", "MK", MACEDONIAN }, // Windows 2000 and later: Macedonian (Macedonia, FYROM)
	{  "ms", "BN", MALAY_BRUNEI_DARUSSALAM }, // Windows 2000 and later: Malay (Brunei Darussalam)
	{  "ms", "MY", MALAY_MALAYSIA }, // Windows 2000 and later: Malay (Malaysia)
	{  "ml", "IN", MALAYALAM }, // Windows XP SP2 and later: Malayalam (India)
	{  "mt", "MT", MALTESE }, // Windows XP SP2 and later: Maltese (Malta)
	{  "mi", "NZ", MAORI }, // Windows XP SP2 and later: Maori (New Zealand)
	{ "arn", "CL", MAPUDUNGUN }, // Windows XP SP2 and later (downloadable); Windows Vista and later: Mapudungun (Chile)
	{  "mr", "IN", MARATHI }, // Windows 2000 and later: Marathi (India)
	{ "moh", "CA", MOHAWK }, // Windows XP SP2 and later (downloadable); Windows Vista and later: Mohawk (Canada)
	{  "mn", "MN", MONGOLIAN }, // Mongolian
	{  "ne", "NP", NEPALI }, // Windows XP SP2 and later (downloadable); Windows Vista and later: Nepali (Nepal)
	{  "nb", "NO", NORWEGIAN_BOKMAL }, // Norwegian (Bokmal, Norway)
	{  "nn", "NO", NORWEGIAN_NYNORSK }, // Norwegian (Nynorsk, Norway)
	{  "oc", "FR", OCCITAN }, // Occitan (France)
	{  "or", "IN", ORIYA }, // Oriya (India)
	{  "ps", "AF", PASHTO }, // Windows XP SP2 and later (downloadable); Windows Vista and later: Pashto (Afghanistan)
	{  "fa", "IR", FARSI }, // Persian (Iran)
	{  "pl", "PL", POLISH }, // Polish (Poland)
	{  "pt", "BR", PORTUGUESE_BRAZILIAN }, // Portuguese (Brazil)
	{  "pt", "PT", PORTUGUESE_STANDARD }, // Portuguese (Portugal)
	{  "pa", "IN", PUNJABI }, // Windows XP and later: Punjabi (India)
	{ "quz", "BO", QUECHUA_BOLIVIA }, // Windows XP SP2 and later: Quechua (Bolivia)
	{ "quz", "EC", QUECHUA_ECUADOR }, // Windows XP SP2 and later: Quechua (Ecuador)
	{ "quz", "PE", QUECHUA_PERU }, // Windows XP SP2 and later: Quechua (Peru)
	{  "ro", "RO", ROMANIAN }, // Romanian (Romania)
	{  "rm", "CH", ROMANSH }, // Windows XP SP2 and later (downloadable); Windows Vista and later: Romansh (Switzerland)
	{  "ru", "RU", RUSSIAN }, // Russian (Russia)
	{ "smn", "FI", SAMI_INARI }, // Windows XP SP2 and later: Sami (Inari, Finland)
	{ "smj", "NO", SAMI_LULE_NORWAY }, // Windows XP SP2 and later: Sami (Lule, Norway)
	{ "smj", "SE", SAMI_LULE_SWEDEN }, // Windows XP SP2 and later: Sami (Lule, Sweden)
	{  "se", "FI", SAMI_NORTHERN_FINLAND }, // Windows XP SP2 and later: Sami (Northern, Finland)
	{  "se", "NO", SAMI_NORTHERN_NORWAY }, // Windows XP SP2 and later: Sami (Northern, Norway)
	{  "se", "SE", SAMI_NORTHERN_SWEDEN }, // Windows XP SP2 and later: Sami (Northern, Sweden)
	{ "sms", "FI", SAMI_SKOLT }, // Windows XP SP2 and later: Sami (Skolt, Finland)
	{ "sma", "NO", SAMI_SOUTHERN_NORWAY }, // Windows XP SP2 and later: Sami (Southern, Norway)
	{ "sma", "SE", SAMI_SOUTHERN_SWEDEN }, // Windows XP SP2 and later: Sami (Southern, Sweden)
	{  "sa", "IN", SANSKRIT }, // Windows 2000 and later: Sanskrit (India)
	{  "sr", "SP", SERBIAN_LATIN }, // Serbian (Latin)
	{  "sr", "SIH", SERBIAN_LATIN_BOSNIA_HERZEGOVINA }, // Serbian (Latin) (Bosnia and Herzegovina)
	{  "sr", "Cyrl_SP", SERBIAN_CYRILLIC }, // Serbian (Cyrillic)
	{  "sr", "Cyrl_SIH", SERBIAN_CYRILLIC_BOSNIA_HERZEGOVINA }, // Serbian (Cyrillic) (Bosnia and Herzegovina)
	{  "ns", "ZA", SESOTHO_SA_LEBOA }, // Windows XP SP2 and later: Sesotho sa Leboa/Northern Sotho (South Africa)
	{  "tn", "ZA", TSWANA }, // Windows XP SP2 and later: Setswana/Tswana (South Africa)
	{  "si", "LK", SINHALA }, // Windows Vista and later: Sinhala (Sri Lanka)
	{  "sk", "SK", SLOVAK }, // Slovak (Slovakia)
	{  "sl", "SI", SLOVENIAN }, // Slovenian (Slovenia)
	{  "es", "AR", SPANISH_ARGENTINA }, // Spanish (Argentina)
	{  "es", "BO", SPANISH_BOLIVIA }, // Spanish (Bolivia)
	{  "es", "CL", SPANISH_CHILE }, // Spanish (Chile)
	{  "es", "CO", SPANISH_COLOMBIA }, // Spanish (Colombia)
	{  "es", "CR", SPANISH_COSTA_RICA }, // Spanish (Costa Rica)
	{  "es", "DO", SPANISH_DOMINICAN_REPUBLIC }, // Spanish (Dominican Republic)
	{  "es", "EC", SPANISH_ECUADOR }, // Spanish (Ecuador)
	{  "es", "SV", SPANISH_EL_SALVADOR }, // Spanish (El Salvador)
	{  "es", "GT", SPANISH_GUATEMALA }, // Spanish (Guatemala)
	{  "es", "HN", SPANISH_HONDURAS }, // Spanish (Honduras)
	{  "es", "MX", SPANISH_MEXICAN }, // Spanish (Mexico)
	{  "es", "NI", SPANISH_NICARAGUA }, // Spanish (Nicaragua)
	{  "es", "PA", SPANISH_PANAMA }, // Spanish (Panama)
	{  "es", "PY", SPANISH_PARAGUAY }, // Spanish (Paraguay)
	{  "es", "PE", SPANISH_PERU }, // Spanish (Peru)
	{  "es", "PR", SPANISH_PUERTO_RICO }, // Spanish (Puerto Rico)
	{  "es", "ES", SPANISH_MODERN_SORT }, // Spanish (Spain)
	{  "es", "ES", SPANISH_TRADITIONAL_SORT }, // Spanish (Spain, Traditional Sort)
	{  "es", "US", SPANISH_UNITED_STATES }, // Windows Vista and later: Spanish (United States)
	{  "es", "UY", SPANISH_URUGUAY }, // Spanish (Uruguay)
	{  "es", "VE", SPANISH_VENEZUELA }, // Spanish (Venezuela)
	{  "sw", "KE", SWAHILI }, // Windows 2000 and later: Swahili (Kenya)
	{  "sv", "FI", SWEDISH_FINLAND }, // Swedish (Finland)
	{  "sv", "SE", SWEDISH }, // Swedish (Sweden)
	{ "syr", "SY", SYRIAC }, // Windows XP and later: Syriac (Syria)
	{  "ta", "IN", TAMIL }, // Windows 2000 and later: Tamil (India)
	{  "tt", "RU", TATAR }, // Windows XP and later: Tatar (Russia)
	{  "te", "IN", TELUGU }, // Windows XP and later: Telugu (India)
	{  "th", "TH", THAI }, // Thai (Thailand)
	{  "bo", "BT", TIBETAN_BHUTAN }, // Windows Vista and later: Tibetan (Bhutan)
	{  "bo", "CN", TIBETAN_PRC }, // Windows Vista and later: Tibetan (PRC)
	{  "tr", "TR", TURKISH }, // Turkish (Turkey)
	{  "tk", "TM", TURKMEN }, // Windows Vista and later: Turkmen (Turkmenistan)
	{  "ug", "CN", UIGHUR }, // Windows Vista and later: Uighur (PRC)
	{  "uk", "UA", UKRAINIAN }, // Ukrainian (Ukraine)
	{ "wen", "DE", UPPER_SORBIAN }, // Windows Vista and later: Upper Sorbian (Germany)
	{  "tr", "IN", URDU_INDIA }, // Urdu (India)
	{  "ur", "PK", URDU }, // Windows 98/Me, Windows 2000 and later: Urdu (Pakistan)
	{  "uz", "UZ", UZBEK_LATIN }, // Uzbek (Latin)
	{  "uz", "Cyrl_UZ", UZBEK_CYRILLIC }, // Uzbek (Cyrillic)
	{  "vi", "VN", VIETNAMESE }, // Windows 98/Me, Windows NT 4.0 and later: Vietnamese (Vietnam)
	{  "cy", "GB", WELSH }, // Windows XP SP2 and later: Welsh (United Kingdom)
	{  "wo", "SN", WOLOF }, // Windows Vista and later: Wolof (Senegal)
	{  "xh", "ZA", XHOSA }, // Windows XP SP2 and later: Xhosa/isiXhosa (South Africa)
	{ "sah", "RU", YAKUT }, // Windows Vista and later: Yakut (Russia)
	{  "ii", "CN", YI }, // Windows Vista and later: Yi (PRC)
	{  "yo", "NG", YORUBA }, // Windows Vista and later: Yoruba (Nigeria)
	{  "zu", "ZA", ZULU } // Windows XP SP2 and later: Zulu/isiZulu (South Africa)
};


typedef struct
{
	// Locale ID
	unsigned int locale;

	// Array of associated keyboard layouts
	unsigned int keyboardLayouts[5];

} localeAndKeyboardLayout;


localeAndKeyboardLayout defaultKeyboardLayouts[] =
{
	{ AFRIKAANS,				{ 0x00000409, 0x00000409, 0x0, 0x0, 0x0 } },
	{ ALBANIAN,				{ 0x0000041c, 0x00000409, 0x0, 0x0, 0x0 } },
	{ ARABIC_SAUDI_ARABIA,			{ 0x00000409, 0x00000401, 0x0, 0x0, 0x0 } },
	{ ARABIC_IRAQ,				{ 0x00000409, 0x00000401, 0x0, 0x0, 0x0 } },
	{ ARABIC_EGYPT,				{ 0x00000409, 0x00000401, 0x0, 0x0, 0x0 } },
	{ ARABIC_LIBYA,				{ 0x0000040c, 0x00020401, 0x0, 0x0, 0x0 } },
	{ ARABIC_ALGERIA,			{ 0x0000040c, 0x00020401, 0x0, 0x0, 0x0 } },
	{ ARABIC_MOROCCO,			{ 0x0000040c, 0x00020401, 0x0, 0x0, 0x0 } },
	{ ARABIC_TUNISIA,			{ 0x0000040c, 0x00020401, 0x0, 0x0, 0x0 } },
	{ ARABIC_OMAN,				{ 0x00000409, 0x00000401, 0x0, 0x0, 0x0 } },
	{ ARABIC_YEMEN,				{ 0x00000409, 0x00000401, 0x0, 0x0, 0x0 } },
	{ ARABIC_SYRIA,				{ 0x00000409, 0x00000401, 0x0, 0x0, 0x0 } },
	{ ARABIC_JORDAN,			{ 0x00000409, 0x00000401, 0x0, 0x0, 0x0 } },
	{ ARABIC_LEBANON,			{ 0x00000409, 0x00000401, 0x0, 0x0, 0x0 } },
	{ ARABIC_KUWAIT,			{ 0x00000409, 0x00000401, 0x0, 0x0, 0x0 } },
	{ ARABIC_UAE,				{ 0x00000409, 0x00000401, 0x0, 0x0, 0x0 } },
	{ ARABIC_BAHRAIN,			{ 0x00000409, 0x00000401, 0x0, 0x0, 0x0 } },
	{ ARABIC_QATAR,				{ 0x00000409, 0x00000401, 0x0, 0x0, 0x0 } },
	{ ARMENIAN,				{ 0x0000042b, 0x00000409, 0x00000419, 0x0, 0x0 } },
	{ AZERI_LATIN,				{ 0x0000042c, 0x0000082c, 0x00000419, 0x0, 0x0 } },
	{ AZERI_CYRILLIC,			{ 0x0000082c, 0x0000042c, 0x00000419, 0x0, 0x0 } },
	{ BASQUE,				{ 0x0000040a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ BELARUSIAN,				{ 0x00000423, 0x00000409, 0x00000419, 0x0, 0x0 } },
	{ BENGALI_INDIA,			{ 0x00000445, 0x00000409, 0x0, 0x0, 0x0 } },
	{ BOSNIAN_LATIN,			{ 0x0000141A, 0x00000409, 0x0, 0x0, 0x0 } },
	{ BULGARIAN,				{ 0x00000402, 0x00000409, 0x0, 0x0, 0x0 } },
	{ CATALAN,				{ 0x0000040a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ CHINESE_TAIWAN,			{ 0x00000404, 0xe0080404, 0xE0010404, 0x0, 0x0 } },
	{ CHINESE_PRC,				{ 0x00000804, 0xe00e0804, 0xe0010804, 0xe0030804, 0xe0040804 } },
	{ CHINESE_HONG_KONG,			{ 0x00000409, 0xe0080404, 0x0, 0x0, 0x0 } },
	{ CHINESE_SINGAPORE,			{ 0x00000409, 0xe00e0804, 0xe0010804, 0xe0030804, 0xe0040804 } },
	{ CHINESE_MACAU,			{ 0x00000409, 0xe00e0804, 0xe0020404, 0xe0080404 } },
	{ CROATIAN,				{ 0x0000041a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ CROATIAN_BOSNIA_HERZEGOVINA,		{ 0x0000041a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ CZECH,				{ 0x00000405, 0x00000409, 0x0, 0x0, 0x0 } },
	{ DANISH,				{ 0x00000406, 0x00000409, 0x0, 0x0, 0x0 } },
	{ DIVEHI,				{ 0x00000409, 0x00000465, 0x0, 0x0, 0x0 } },
	{ DUTCH_STANDARD,			{ 0x00020409, 0x00000413, 0x00000409, 0x0, 0x0 } },
	{ DUTCH_BELGIAN,			{ 0x00000813, 0x00000409, 0x0, 0x0, 0x0 } },
	{ ENGLISH_UNITED_STATES,		{ 0x00000409, 0x0, 0x0, 0x0, 0x0 } },
	{ ENGLISH_UNITED_KINGDOM,		{ 0x00000809, 0x0, 0x0, 0x0, 0x0 } },
	{ ENGLISH_AUSTRALIAN,			{ 0x00000409, 0x0, 0x0, 0x0, 0x0 } },
	{ ENGLISH_CANADIAN,			{ 0x00000409, 0x00011009, 0x00001009, 0x0, 0x0 } },
	{ ENGLISH_NEW_ZEALAND,			{ 0x00000409, 0x0, 0x0, 0x0, 0x0 } },
	{ ENGLISH_IRELAND,			{ 0x00001809, 0x00011809, 0x0, 0x0, 0x0 } },
	{ ENGLISH_SOUTH_AFRICA,			{ 0x00000409, 0x0, 0x0, 0x0, 0x0 } },
	{ ENGLISH_JAMAICA,			{ 0x00000409, 0x0, 0x0, 0x0, 0x0 } },
	{ ENGLISH_CARIBBEAN,			{ 0x00000409, 0x0, 0x0, 0x0, 0x0 } },
	{ ENGLISH_BELIZE,			{ 0x00000409, 0x0, 0x0, 0x0, 0x0 } },
	{ ENGLISH_TRINIDAD,			{ 0x00000409, 0x0, 0x0, 0x0, 0x0 } },
	{ ENGLISH_ZIMBABWE,			{ 0x00000409, 0x0, 0x0, 0x0, 0x0 } },
	{ ENGLISH_PHILIPPINES,			{ 0x00000409, 0x0, 0x0, 0x0, 0x0 } },
	{ ESTONIAN,				{ 0x00000425, 0x0, 0x0, 0x0, 0x0 } },
	{ FAEROESE,				{ 0x00000406, 0x00000409, 0x0, 0x0, 0x0 } },
	{ FARSI,				{ 0x00000409, 0x00000429, 0x00000401, 0x0, 0x0 } },
	{ FINNISH,				{ 0x0000040b, 0x00000409, 0x0, 0x0, 0x0 } },
	{ FRENCH_STANDARD,			{ 0x0000040c, 0x00000409, 0x0, 0x0, 0x0 } },
	{ FRENCH_BELGIAN,			{ 0x0000080c, 0x00000409, 0x0, 0x0, 0x0 } },
	{ FRENCH_CANADIAN,			{ 0x00000C0C, 0x00011009, 0x00000409, 0x0, 0x0 } },
	{ FRENCH_SWISS,				{ 0x0000100c, 0x00000409, 0x0, 0x0, 0x0 } },
	{ FRENCH_LUXEMBOURG,			{ 0x0000040c, 0x00000409, 0x0, 0x0, 0x0 } },
	{ FRENCH_MONACO,			{ 0x0000040c, 0x00000409, 0x0, 0x0, 0x0 } },
	{ GEORGIAN,				{ 0x00000437, 0x00000409, 0x00000419, 0x0, 0x0 } },
	{ GALICIAN,				{ 0x0000040a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ GERMAN_STANDARD,			{ 0x00000407, 0x00000409, 0x0, 0x0, 0x0 } },
	{ GERMAN_SWISS,				{ 0x00000807, 0x00000409, 0x0, 0x0, 0x0 } },
	{ GERMAN_AUSTRIAN,			{ 0x00000407, 0x00000409, 0x0, 0x0, 0x0 } },
	{ GERMAN_LUXEMBOURG,			{ 0x00000407, 0x00000409, 0x0, 0x0, 0x0 } },
	{ GERMAN_LIECHTENSTEIN,			{ 0x00000407, 0x00000409, 0x0, 0x0, 0x0 } },
	{ GREEK,				{ 0x00000408, 0x00000409, 0x0, 0x0, 0x0 } },
	{ GUJARATI,				{ 0x00000409, 0x00000447, 0x00010439, 0x0, 0x0 } },
	{ HEBREW,				{ 0x00000409, 0x0000040d, 0x0, 0x0, 0x0 } },
	{ HINDI,				{ 0x00000409, 0x00010439, 0x00000439, 0x0, 0x0 } },
	{ HUNGARIAN,				{ 0x0000040e, 0x00000409, 0x0, 0x0, 0x0 } },
	{ ICELANDIC,				{ 0x0000040f, 0x00000409, 0x0, 0x0, 0x0 } },
	{ INDONESIAN,				{ 0x00000409, 0x00000409, 0x0, 0x0, 0x0 } },
	{ ITALIAN_STANDARD,			{ 0x00000410, 0x00000409, 0x0, 0x0, 0x0 } },
	{ ITALIAN_SWISS,			{ 0x00000410, 0x00000409, 0x0, 0x0, 0x0 } },
	{ JAPANESE,				{ 0xe0010411, 0x0, 0x0, 0x0, 0x0 } },
	{ KANNADA,				{ 0x00000409, 0x0000044b, 0x00010439, 0x0, 0x0 } },
	{ KAZAKH,				{ 0x0000043f, 0x00000409, 0x00000419, 0x0, 0x0 } },
	{ KONKANI,				{ 0x00000409, 0x00000439, 0x0, 0x0, 0x0 } },
	{ KOREAN,				{ 0xE0010412, 0x0, 0x0, 0x0, 0x0 } },
	{ KYRGYZ,				{ 0x00000440, 0x00000409, 0x0, 0x0, 0x0 } },
	{ LATVIAN,				{ 0x00010426, 0x0, 0x0, 0x0, 0x0 } },
	{ LITHUANIAN,				{ 0x00010427, 0x0, 0x0, 0x0, 0x0 } },
	{ MACEDONIAN,				{ 0x0000042f, 0x00000409, 0x0, 0x0, 0x0 } },
	{ MALAY_MALAYSIA,			{ 0x00000409, 0x0, 0x0, 0x0, 0x0 } },
	{ MALAY_BRUNEI_DARUSSALAM,		{ 0x00000409, 0x0, 0x0, 0x0, 0x0 } },
	{ MALAYALAM,				{ 0x00000409, 0x0000044c, 0x0, 0x0, 0x0 } },
	{ MALTESE,				{ 0x00000409, 0x0000043a, 0x0, 0x0, 0x0 } },
	{ MAORI,				{ 0x00000409, 0x00000481, 0x0, 0x0, 0x0 } },
	{ MARATHI,				{ 0x00000409, 0x0000044e, 0x00000439, 0x0, 0x0 } },
	{ MONGOLIAN,				{ 0x00000450, 0x00000409, 0x0, 0x0, 0x0 } },
	{ NORWEGIAN_BOKMAL,			{ 0x00000414, 0x00000409, 0x0, 0x0, 0x0 } },
	{ NORWEGIAN_NYNORSK,			{ 0x00000414, 0x00000409, 0x0, 0x0, 0x0 } },
	{ POLISH,				{ 0x00010415, 0x00000415, 0x00000409, 0x0, 0x0 } },
	{ PORTUGUESE_BRAZILIAN,			{ 0x00000416, 0x00000409, 0x0, 0x0, 0x0 } },
	{ PORTUGUESE_STANDARD,			{ 0x00000816, 0x00000409, 0x0, 0x0, 0x0 } },
	{ PUNJABI,				{ 0x00000409, 0x00000446, 0x00010439, 0x0, 0x0 } },
	{ QUECHUA_BOLIVIA,			{ 0x00000409, 0x0000080A, 0x0, 0x0, 0x0 } },
	{ QUECHUA_ECUADOR,			{ 0x00000409, 0x0000080A, 0x0, 0x0, 0x0 } },
	{ QUECHUA_PERU,				{ 0x00000409, 0x0000080A, 0x0, 0x0, 0x0 } },
	{ ROMANIAN,				{ 0x00000418, 0x00000409, 0x0, 0x0, 0x0 } },
	{ RUSSIAN,				{ 0x00000419, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SAMI_INARI,				{ 0x0001083b, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SAMI_LULE_NORWAY,			{ 0x0000043b, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SAMI_LULE_SWEDEN,			{ 0x0000083b, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SAMI_NORTHERN_FINLAND,		{ 0x0001083b, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SAMI_NORTHERN_NORWAY,			{ 0x0000043b, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SAMI_NORTHERN_SWEDEN,			{ 0x0000083b, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SAMI_SKOLT,				{ 0x0001083b, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SAMI_SOUTHERN_NORWAY,			{ 0x0000043b, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SAMI_SOUTHERN_SWEDEN,			{ 0x0000083b, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SANSKRIT,				{ 0x00000409, 0x00000439, 0x0, 0x0, 0x0 } },
	{ SERBIAN_LATIN,			{ 0x0000081a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SERBIAN_LATIN_BOSNIA_HERZEGOVINA,	{ 0x0000081a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SERBIAN_CYRILLIC,			{ 0x00000c1a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SERBIAN_CYRILLIC_BOSNIA_HERZEGOVINA,	{ 0x00000c1a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SLOVAK,				{ 0x0000041b, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SLOVENIAN,				{ 0x00000424, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_TRADITIONAL_SORT,		{ 0x0000040a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_MEXICAN,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_MODERN_SORT,			{ 0x0000040a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_GUATEMALA,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_COSTA_RICA,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_PANAMA,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_DOMINICAN_REPUBLIC,		{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_VENEZUELA,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_COLOMBIA,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_PERU,				{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_ARGENTINA,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_ECUADOR,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_CHILE,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_URUGUAY,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_PARAGUAY,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_BOLIVIA,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_EL_SALVADOR,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_HONDURAS,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_NICARAGUA,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SPANISH_PUERTO_RICO,			{ 0x0000080a, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SWAHILI,				{ 0x00000409, 0x0, 0x0, 0x0, 0x0 } },
	{ SWEDISH,				{ 0x0000041d, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SWEDISH_FINLAND,			{ 0x0000041d, 0x00000409, 0x0, 0x0, 0x0 } },
	{ SYRIAC,				{ 0x00000409, 0x0000045a, 0x0, 0x0, 0x0 } },
	{ TAMIL,				{ 0x00000409, 0x00000449, 0x0, 0x0, 0x0 } },
	{ TATAR,				{ 0x00000444, 0x00000409, 0x00000419, 0x0, 0x0 } },
	{ TELUGU,				{ 0x00000409, 0x0000044a, 0x00010439, 0x0, 0x0 } },
	{ THAI,					{ 0x00000409, 0x0000041e, 0x0, 0x0, 0x0 } },
	{ TSWANA,				{ 0x00000409, 0x0000041f, 0x0, 0x0, 0x0 } },
	{ UKRAINIAN,				{ 0x00000422, 0x00000409, 0x0, 0x0, 0x0 } },
	{ TURKISH,				{ 0x0000041f, 0x0000041f, 0x0, 0x0, 0x0 } },
	{ UKRAINIAN,				{ 0x00000422, 0x00000409, 0x0, 0x0, 0x0 } },
	{ URDU,					{ 0x00000401, 0x00000409, 0x0, 0x0, 0x0 } },
	{ UZBEK_LATIN,				{ 0x00000409, 0x00000843, 0x00000419, 0x0, 0x0 } },
	{ UZBEK_CYRILLIC,			{ 0x00000843, 0x00000409, 0x00000419, 0x0, 0x0 } },
	{ VIETNAMESE,				{ 0x00000409, 0x0000042a, 0x0, 0x0, 0x0 } },
	{ WELSH,				{ 0x00000452, 0x00000809, 0x0, 0x0, 0x0 } },
	{ XHOSA,				{ 0x00000409, 0x00000409, 0x0, 0x0, 0x0 } },
};

/*
Time zones, taken from Windows Server 2008

(GMT -12:00) International Date Line West
(GMT -11:00) Midway Island, Samoa
(GMT -10:00) Hawaii
(GMT -09:00) Alaska
(GMT -08:00) Pacific Time (US & Canada)
(GMT -08:00) Tijuana, Baja California
(GMT -07:00) Arizona
(GMT -07:00) Chihuahua, La Paz, Mazatlan
(GMT -07:00) Mountain Time (US & Canada)
(GMT -06:00) Central America
(GMT -06:00) Central Time (US & Canada)
(GMT -06:00) Guadalajara, Mexico City, Monterrey
(GMT -06:00) Saskatchewan
(GMT -05:00) Bogota, Lima, Quito, Rio Branco
(GMT -05:00) Eastern Time (US & Canada)
(GMT -05:00) Indiana (East)
(GMT -04:30) Caracas
(GMT -04:00) Atlantic Time (Canada)
(GMT -04:00) La Paz
(GMT -04:00) Manaus
(GMT -04:00) Santiago
(GMT -03:30) Newfoundland
(GMT -03:00) Brasilia
(GMT -03:00) Buenos Aires
(GMT -03:00) Georgetown
(GMT -03:00) Greenland
(GMT -03:00) Montevideo
(GMT -02:00) Mid-Atlantic
(GMT -01:00) Azores
(GMT -01:00) Cape Verde Is.
(GMT +00:00) Casablanca
(GMT +00:00) Greenwich Mean Time: Dublin, Edinburgh, Lisbon, London
(GMT +00:00) Monrovia, Reykjavik
(GMT +01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna
(GMT +01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague
(GMT +01:00) Brussels, Copenhagen, Madrid, Paris
(GMT +01:00) Sarajevo, Skopje, Warsaw, Zagreb
(GMT +01:00) West Central Africa
(GMT +02:00) Amman
(GMT +02:00) Athens, Bucharest, Istanbul
(GMT +02:00) Beirut
(GMT +02:00) Cairo
(GMT +02:00) Harare, Pretoria
(GMT +02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius
(GMT +02:00) Jerusalem
(GMT +02:00) Minsk
(GMT +02:00) Windhoek
(GMT +03:00) Baghdad
(GMT +03:00) Kuwait, Riyadh
(GMT +03:00) Moscow, St. Petersburg, Volgograd
(GMT +03:00) Nairobi
(GMT +03:00) Tbilisi
(GMT +03:30) Tehran
(GMT +04:00) Abu Dhabi, Muscat
(GMT +04:00) Baku
(GMT +04:00) Port Louis
(GMT +04:00) Yerevan
(GMT +04:30) Kabul
(GMT +05:00) Ekaterinburg
(GMT +05:00) Islamabad, Karachi
(GMT +05:00) Tashkent
(GMT +05:30) Chennai, Kolkata, Mumbai, New Delhi
(GMT +05:30) Sri Jayawardenepura
(GMT +05:45) Kathmandu
(GMT +06:00) Almaty, Novosibirsk
(GMT +06:00) Astana, Dhaka
(GMT +06:30) Yangon (Rangoon)
(GMT +07:00) Bangkok, Hanoi, Jakarta
(GMT +07:00) Krasnoyarsk
(GMT +08:00) Beijing, Chongqing, Hong Kong, Urumqi
(GMT +08:00) Irkutsk, Ulaan Bataar
(GMT +08:00) Kuala Lumpur, Singapore
(GMT +08:00) Perth
(GMT +08:00) Taipei
(GMT +09:00) Osaka, Sapporo, Tokyo
(GMT +09:00) Seoul
(GMT +09:00) Yakutsk
(GMT +09:30) Adelaide
(GMT +09:30) Darwin
(GMT +10:00) Brisbane
(GMT +10:00) Canberra, Melbourne, Sydney
(GMT +10:00) Guam, Port Moresby
(GMT +10:00) Hobart, Vladivostok
(GMT +11:00) Magadan, Solomon Is., New Caledonia
(GMT +12:00) Auckland, Wellington
(GMT +12:00) Fiji, Kamchatka, Marshall Is.
(GMT +13:00) Nuku'alofa
*/

#endif // __LOCALES_H

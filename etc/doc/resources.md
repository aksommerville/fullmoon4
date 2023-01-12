# Full Moon Resources

# Image

src/data/image/{id}[-name][.qualifier][.lang].png

{id} is a decimal integer 1..255 without leading zeroes.
{name} is anything except dot and will not necessarily be preserved.
{qualifier} is not yet used. Eventually will be things like "32px", "8px", "bw8px", etc. Must not be two letters.
{lang} is the ISO 631 language code.

# Map

src/data/map/{id}

{id} is a decimal integer 1..65535 without leading zeroes.

# String

src/data/string/{lang}

{lang} is a 2-character ISO 631 code eg "en"=English, "ru"=Russian.

Each line in the file is one string resource. First the decimal string id 1..65535, then the text, UTF-8.

Strings can't contain newlines.

# Song

src/data/song/{id}.mid

{id} is a decimal integer 1..255 without leading zeroes.

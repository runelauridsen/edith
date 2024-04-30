:: https://stackoverflow.com/a/66238793
pyftsubset OpenSans-Bold.ttf        --output-file=OpenSans-Stripped-Bold.ttf        --unicodes=U+0000-00FF
pyftsubset OpenSans-BoldItalic.ttf  --output-file=OpenSans-Stripped-BoldItalic.ttf  --unicodes=U+0000-00FF
pyftsubset OpenSans-Italic.ttf      --output-file=OpenSans-Stripped-Italic.ttf      --unicodes=U+0000-00FF
pyftsubset OpenSans-Regular.ttf     --output-file=OpenSans-Stripped-Regular.ttf     --unicodes=U+0000-00FF

cembed OpenSans-Stripped-Bold.ttf           opensans_stripped_bold.h            opensans_stripped_bold_data
cembed OpenSans-Stripped-BoldItalic.ttf     opensans_stripped_bold_italic.h     opensans_stripped_bold_italic_data
cembed OpenSans-Stripped-Italic.ttf         opensans_stripped_italic.h          opensans_stripped_italic_data
cembed OpenSans-Stripped-Regular.ttf        opensans_stripped_regular.h         opensans_stripped_regular_data
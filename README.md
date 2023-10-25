# xschem-gaw

## Quick Install

To install gaw perform these commands:

```bash
./configure
make
sudo make install
```

If previous method fails try this:

```bash
aclocal
autoconf
autoheader
automake --add-missing
./configure
make
sudo make install
```

## Fix gettext issues

If you get this kind of message
```
./configure: line 5347: syntax error near unexpected token `external'
./configure: line 5347: `AM_GNU_GETTEXT(external)'
```
This might be the issue, as found by user [Anthony Wall](https://github.com/anthonyjwall):

*My gettext was from libglib2.0-dev, so the installed library referenced AM_GLIB_GNU_GETTEXT_* instead of AM_GNU_GETTEXT_*.
I ran sudo apt install gettext (different to previously ran sudo apt install libglib2.0-dev), re-cloned and the install ran correctly.*

If you get this kind of error:
```
*** error: gettext infrastructure mismatch: using a Makefile.in.in from gettext version 0.20 but the autoconf macros are from gettext version 0.19
``` 
there is a gettext version mismatch between the expected one and the one installed on your system.
edit the file po/Makefile.in.in and locate the following line:
```
GETTEXT_MACRO_VERSION = 0.20
```
change the number to 0.19 or 0.18 or whatever the version number of gettext installed on your system. Taking above error as an example you should
replace 0.20 with 0.19.
After this change rebuild the configure, run configure and make:
```
aclocal && automake --add-missing && autoconf
./configure
make
sudo make install
```

## More Information

gaw3-20200922 fork with patches to improve remote commands sent from xschem to display waveforms.
Original work from Hervé Quillévéré

- [http://www.rvq.fr](http://www.rvq.fr)
- [http://gaw.tuxfamily.org](http://gaw.tuxfamily.org)

For use and install information on GAW see [README](https://github.com/StefanSchippers/xschem-gaw/blob/main/README)

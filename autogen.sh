#! /bin/sh -x

aclocal || exit 1
autoheader --force || exit 2
libtoolize -c --automake --force || glibtoolize -c --automake --force || exit 3
automake --add-missing --copy --include-deps || exit 4
autoconf || exit 5

rm -rf autom4te.cache

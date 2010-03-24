#!/bin/sh
indent -bli0 -i8 -cli8 -npcs -l100 `find * -name '*.[ch]' | grep -v ^asn1/`

#!/bin/bash
clang-format -i $(find . -not -path '*/libnica/*' -name '*.[ch]')

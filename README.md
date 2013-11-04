redis-v8
==============
Short description here

Building from source (CentOS 6.4 x86_64 minimal)
====

yum install -y @development-tools

yum install -y make

yum install -y vim-common

yum install -y svn

rpm -Uvh http://elrepo.org/elrepo-release-6-5.el6.elrepo.noarch.rpm

yum --enablerepo=elrepo-extras install -y clang

make

make install

Contributors
============
Arseniy Pavlenko (arseniy@tikalk.com)

Alexander Faitelson (treeskar@gmail.com)

LICENSE
=======
Copyright (c) 2013 Arseniy Pavlenko <h0x91b@gmail.com>

Copyright (c) 2006-2012, Salvatore Sanfilippo
All rights reserved.

Copyright 2006-2012, the V8 project authors. All rights reserved.

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

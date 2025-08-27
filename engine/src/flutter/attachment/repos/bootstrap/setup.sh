# Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_KHZG file.
#! /bin/sh

chmod -R +w  .vpython*
#cp -a  .vpython*  ~/

cp -a .vpython-root  .vpython-root-new
find .vpython-root-new/ -type l -exec ls -l {} \; | awk '{
	    D = ENVIRON["PWD"];
	    TARGET = $9;
	    s = index($11, "depot");
	    T = D "/" substr($11, s);
	   #print $9 "->" T;
	   cmd = "rm -f " TARGET;
	   system(cmd);
	   cmd = "ln -sf " T " " TARGET;
	   system(cmd);
}'

rm -rf ~/.vpython-root
cp -a .vpython-root-new   ~/.vpython-root&&  rm -rf .vpython-root-new
cp -a  .vpython_cipd_cache  ~/

chmod -R +w  ~/.vpython*

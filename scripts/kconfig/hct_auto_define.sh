#!/bin/bash


auto_define="`cat $1/kernel/configs/hct_auto_define_list`"

echo "auto_define=$auto_define"
echo "`pwd`"
config_file=$2
#autoconf_h="include/generated/autoconf.h" 

hct_auto_define_dir="include/generated"
hct_auto_define_h="${hct_auto_define_dir}/hct_auto_define_list.h"

[ ! -d  "$hct_auto_define_dir" ] && mkdir -p "$hct_auto_define_dir"

cat << EOF > "$hct_auto_define_h"
/*
 * Automatically generated file; DO NOT EDIT.
 * HCT Configuration 
 * 如果想生成自动宏,请添加在 kernel-3.18/kernel/configs/hct_auto_define_list
 * 生成格式:#define AUTO_宏_宏定义 1
 *
 */

#ifndef __HCT_AUTO_DEFINE_LIST_H__
#define __HCT_AUTO_DEFINE_LIST_H__

EOF

if [ ! -f  $1/kernel/configs/hct_auto_define_list ];then
	echo "#endif" >> "$hct_auto_define_h"
	exit 
fi

for each in $auto_define;do
	str=`grep  "^$each=" -r $2`
	if [ ! -z "$str" ];then
		suffix="`echo $str | cut -d '=' -f 2 | sed 's/"//g' | tr '[a-z]' '[A-Z]'`"
		for each1 in $suffix;do
			echo "#define AUTO_${each}_${each1} 1"
			echo "#define AUTO_${each}_${each1} 1" >> "$hct_auto_define_h"
		done
	fi
done

echo "#endif" >> "$hct_auto_define_h"

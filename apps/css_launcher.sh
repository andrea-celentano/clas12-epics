#!/bin/sh

css_share_path=../css_share

css \
    -pluginCustomization $css_share_path/common/prefs/plugin_customization.ini \
	-share_link $css_share_path=/CLAS12_Share \
	-nosplash &


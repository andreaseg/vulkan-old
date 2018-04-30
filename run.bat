@echo off
set VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_standard_validation
cd target
call "./main.exe"
cd ..
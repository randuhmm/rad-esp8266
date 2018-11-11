#!/usr/bin/env bash

# Fixing a Problem with the boards.txt
# ld: cannot open linker script file {build.flash_ld}: No such file or directory
# choosing "4M (3M SPIFFS)" Configuration
file=`find $HOME/.arduino15/packages/esp8266/ -type f -name 'boards.txt'`
echo Changing $file

sed -i.bak 's/nodemcu.menu.FlashSize.4M3M/nodemcu/g' $file
sed -i.bak 's/nodemcuv2.menu.FlashSize.4M3M/nodemcuv2/g' $file
sed -i.bak 's/d1_mini.menu.FlashSize.4M3M/d1_mini/g' $file
sed -i.bak 's/esp8285.menu.FlashSize.1M128/esp8285/g' $file

for i in nodemcu nodemcuv2 d1_mini esp8285
do
	echo "${i}.build.f_cpu=80000000L" >> $file
done


function build_examples()
{
  # track the exit code for this platform
  local exit_code=0
  # loop through results and add them to the array
  examples=($(find $PWD/examples/ -name "*.pde" -o -name "*.ino"))

  # get the last example in the array
  local last="${examples[@]:(-1)}"

  # loop through example sketches
  for example in "${examples[@]}"; do

    # store the full path to the example's sketch directory
    local example_dir=$(dirname $example)

    # store the filename for the example without the path
    local example_file=$(basename $example)

    echo "$example_file: "
    local sketch="$example_dir/$example_file"
    echo "$sketch"
    #arduino -v --verbose-build --verify $sketch

    # verify the example, and save stdout & stderr to a variable
    # we have to avoid reading the exit code of local:
    # "when declaring a local variable in a function, the local acts as a command in its own right"
    local build_stdout
    build_stdout=$(arduino --verify $sketch 2>&1)

    # echo output if the build failed
    if [ $? -ne 0 ]; then
      # heavy X
      echo -e "\xe2\x9c\x96"
      echo -e "----------------------------- DEBUG OUTPUT -----------------------------\n"
      echo "$build_stdout"
      echo -e "\n------------------------------------------------------------------------\n"

      # mark as fail
      exit_code=1

    else
      # heavy checkmark
      echo -e "\xe2\x9c\x93"
    fi
  done

  return $exit_code
}

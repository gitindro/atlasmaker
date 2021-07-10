# atlasmaker
Command line tool(C++) to build atlas from png images in a folder. Also spits out a json file with atlas info.

# Usage - 
**Example:**  --path  "D:\pngs" --width 2048 --height 2048  --prefix "objs"

path = path to Directory containing individual pngs

width - max width of output atlas.

height - max height of output atlas.

prefix - Prefix for Output file names.


To Build the project you will need CImg - https://github.com/dtschump/CImg (you will need a png lib for CImg to work with pngs) and Cereal - https://uscilab.github.io/cereal/.

# ToDo - 
add MaxRect packing as option.

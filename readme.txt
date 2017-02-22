Warping (MATLAB):

> In the script run_multiple_iter_warp.m, set the paths for the input images (sketchPath), the manual correspondences (corrPath), the masks (maskPath), and the output (outPath).
> Set the prefix for th input images (imgNamePrefix), and a sequence (seqImage), such that your images are imgNamePrefix_{{seqImage(1)}}, imgNamePrefix{{seqImage(2)}}, ...
> Run the script run_multiple_iter_warp.m.

Rendering (Qt/C++):

> Build the Qt code. I have compiled it using Qt5 (v5.4 to 5.8 should work, at least). There are no significant dependencies. The only one I know of is OpenGL4 (might work with OpenGL3, but wouldn't work with OpenGL2).
> The MATLAB code would have produced an input file with references to all the other files (triangulation topology as *_tri.txt, and geometry as *_vert*.txt). The input file is of the format:
concat( outNamePrefix, concat(seqImage[1], ..., seqImage[n]), "_input.txt" )
> Run using the full path of this input file as the only command-line argument. For example, 
.\qt_meshrenderer.exe C:\arorar\Documents\ironnew\ironnew_267_input.txt

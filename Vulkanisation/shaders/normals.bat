@echo off
glslang -V normal.vert -o normal.vert.spv
glslang -V normal.geom -o normal.geom.spv
glslang -V normal.frag -o normal.frag.spv
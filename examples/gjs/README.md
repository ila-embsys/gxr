# Running 'main.ts' demo

Before running the demo, set the environments

```shell
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(readlink -f ../../builddir/src)
export GI_TYPELIB_PATH=$(readlink -f ../../builddir/src)
export GI_GIR_PATH=$(readlink -f ../../builddir/src)
```

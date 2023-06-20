choosing between various simd libraries to add simd support

axes:
- does it have recent activity?
- overloaded operators
- bitwise operators
- reference / non-simd implementations
- static dispatch
- dynamic dispatch

| library | active | operators | bitwise | non-simd | static | dynamic |
| --- | --- | --- | --- | --- | --- | --- |
| [highway](https://github.com/google/highway) | :heavy_check_mark: | :grey_question: | :grey_question: | :grey_question: | :heavy_check_mark: | :heavy_check_mark: |
| [simde](https://github.com/simd-everywhere/simde) | :heavy_check_mark: | :x: | :x: | :heavy_check_mark: | :heavy_check_mark: | :x: |
| [xsimd](https://github.com/xtensor-stack/xsimd) | :heavy_check_mark: | :heavy_check_mark: | :grey_question: | :grey_question: | :heavy_check_mark: | [:heavy_check_mark:](https://xsimd.readthedocs.io/en/latest/api/dispatching.html) |
| [libsimdpp](https://github.com/p12tic/libsimdpp) | :x: | :heavy_check_mark: | :heavy_check_mark: | :grey_question: | :heavy_check_mark: | [:heavy_check_mark:](http://p12tic.github.io/libsimdpp/v2.2-dev/libsimdpp/w/arch/dispatch.html) |
| [std-simd](https://github.com/VcDevel/std-simd) | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | [:x:](https://en.cppreference.com/w/cpp/experimental/simd/deduce) |

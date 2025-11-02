#To Build the project :#

```apt update```
```apt install -y build-essential cmake git openjdk-17-jdk```
```apt install libopencv-dev```
```curl -L -o /workspace/ImageDSL/build/antlr-4.13.0-complete.jar https://www.antlr.org/download/antlr-4.13.0-complete.jar```

```cd ImageDSL```
```mkdir build && cd build```
```cmake ..```
```cmake --build . ``` 

#To Run the project :#

```./image-dsl ../test/sample.imgdsl ```

#For running without optimizations :
```./image-dsl ../test/sample.imgdsl -mem2reg=false -cse=false -cf=false -copyprop=false```

#For benchmarking :
```./image-dsl ../test/sample.imgdsl -benchmark=true```
```./image-dsl ../test/sample.imgdsl -benchmark=true -mem2reg=false -cse=false -cf=false -copyprop=false```


#To Run the Functional Tests
1. ```g++ main.cpp -o test_harness `pkg-config --cflags --libs opencv4```
2. ```./test_harness {path_to_directory_with_images} {path_to_output_image} {method}```  
    - Example: ```./test_harness /root/data/samples/images/img-r.jpeg /root/data/samples/images/output.jpg brighten```

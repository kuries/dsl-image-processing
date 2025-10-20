#To Build the project :

apt update  
apt install -y build-essential cmake git openjdk-17-jdk  
curl -L -o /workspace/dsl-image-processing/build/antlr-4.13.0-complete.jar https://www.antlr.org/download/antlr-4.13.0-complete.jar  

cd dsl-image-processing  
mkdir build && cd build  
cmake ..  
cmake --build .    
                    (or cmake --build . --parallel to make it faster)  



#To Run the project :

cd build  
./image-dsl ../test/sample.imgdsl   
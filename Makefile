CXX=clang++
CXXFLAGS=-std=c++20 -O3
TARGET=cppgrad

all: $(TARGET)

$(TARGET): cppgrad.o
	$(CXX) $(CXXFLAGS) -o $(TARGET) cppgrad.o

cppgrad.o: cppgrad.cpp
	$(CXX) $(CXXFLAGS) -c cppgrad.cpp

clean:
	rm -f $(TARGET) *.o

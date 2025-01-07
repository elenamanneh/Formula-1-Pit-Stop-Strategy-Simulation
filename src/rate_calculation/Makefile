CXX = g++
CXXFLAGS = -std=c++17 -Wall

TARGET = rate_calculation

SRC = rate_calculation.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

all: $(TARGET)

# Clean build files
clean:
	rm -f $(TARGET)

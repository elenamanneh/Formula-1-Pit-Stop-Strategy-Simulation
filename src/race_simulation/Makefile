CXX = mpicxx
CXXFLAGS = -std=c++17 -Wall -fopenmp -Iinclude
PROFILE_FLAGS = -pg

TARGETS = stint_time_calculator generate_strategies

all: $(TARGETS)

stint_time_calculator: stint_time_calculator.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

generate_strategies: generate_strategies.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

profile: CXXFLAGS += $(PROFILE_FLAGS)
profile: $(TARGETS)

clean:
	rm -f $(TARGETS) gmon.out


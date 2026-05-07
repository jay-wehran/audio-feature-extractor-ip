CXX      = g++
CXXFLAGS = -std=c++14 -Wall -Wextra
TARGET   = feature_ip_tb
SRCS     = tb_feature_ip.cpp feature_ip.cpp

$(TARGET): $(SRCS) feature_ip.hpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
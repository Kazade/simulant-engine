
LINK_LIBRARIES(
    kglt
    ${KAZMATH_LIBRARIES}
)

ADD_EXECUTABLE(sample sample.cpp)
ADD_EXECUTABLE(q2_sample q2bsp_sample.cpp)

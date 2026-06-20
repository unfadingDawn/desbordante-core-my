set(NAME cdd.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cdd_verifier.cpp)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::dd
            better-enums
            ${DESBORDANTE_PREFIX}::model::table
)

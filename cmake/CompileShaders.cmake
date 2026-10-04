include_guard(GLOBAL)

function(add_spirv_shader_library)
    set(options)

    set(oneValueArgs
        TARGET
        DIRECTORY
        TARGET_ENV
        NAMESPACE
    )

    set(multiValueArgs
        INCLUDE_DIRECTORIES
        DEFINITIONS
    )

    cmake_parse_arguments(
        SHADER
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    # =========================================================================
    # Validate arguments
    # =========================================================================

    if(NOT SHADER_TARGET)
        message(FATAL_ERROR
            "add_spirv_shader_library: TARGET is required"
        )
    endif()

    if(NOT SHADER_DIRECTORY)
        message(FATAL_ERROR
            "add_spirv_shader_library: DIRECTORY is required"
        )
    endif()

    if(NOT SHADER_TARGET_ENV)
        message(FATAL_ERROR
            "add_spirv_shader_library: TARGET_ENV is required"
        )
    endif()

    if(NOT IS_DIRECTORY "${SHADER_DIRECTORY}")
        message(FATAL_ERROR
            "Shader directory does not exist: ${SHADER_DIRECTORY}"
        )
    endif()

    if(NOT SHADER_NAMESPACE)
        set(SHADER_NAMESPACE "${SHADER_TARGET}")
    endif()

    #
    # IMPORTANT:
    #
    # Do NOT sanitize SHADER_NAMESPACE.
    #
    # We want:
    #
    #     VISR::Shaders::OpenGL
    #
    # to remain exactly that.
    #

    # =========================================================================
    # Find glslangValidator
    # =========================================================================

    find_program(
        GLSLANG_VALIDATOR
        NAMES
            glslangValidator
        HINTS
            "$ENV{VULKAN_SDK}/Bin"
            "$ENV{VULKAN_SDK}/bin"
    )

    if(NOT GLSLANG_VALIDATOR)
        message(FATAL_ERROR
            "glslangValidator was not found.\n"
            "Install glslang or the Vulkan SDK."
        )
    endif()

    # =========================================================================
    # Find GLSL shaders
    # =========================================================================
    #
    # Supports:
    #
    #     shader.vert
    #     shader.frag
    #     shader.comp
    #     shader.geom
    #     shader.tesc
    #     shader.tese
    #
    # and:
    #
    #     shader.vert.glsl
    #     shader.frag.glsl
    #     ...
    #
    # CONFIGURE_DEPENDS makes adding/removing a shader cause CMake to
    # automatically reconfigure.
    #

    file(
        GLOB_RECURSE
        SHADER_SOURCES
        CONFIGURE_DEPENDS

        "${SHADER_DIRECTORY}/*.vert"
        "${SHADER_DIRECTORY}/*.frag"
        "${SHADER_DIRECTORY}/*.comp"
        "${SHADER_DIRECTORY}/*.geom"
        "${SHADER_DIRECTORY}/*.tesc"
        "${SHADER_DIRECTORY}/*.tese"

        "${SHADER_DIRECTORY}/*.vert.glsl"
        "${SHADER_DIRECTORY}/*.frag.glsl"
        "${SHADER_DIRECTORY}/*.comp.glsl"
        "${SHADER_DIRECTORY}/*.geom.glsl"
        "${SHADER_DIRECTORY}/*.tesc.glsl"
        "${SHADER_DIRECTORY}/*.tese.glsl"
    )

    list(SORT SHADER_SOURCES)

    if(NOT SHADER_SOURCES)
        message(WARNING
            "No GLSL shaders found in ${SHADER_DIRECTORY}"
        )
    endif()

    # =========================================================================
    # Output paths
    # =========================================================================

    set(
        GENERATED_ROOT
        "${CMAKE_CURRENT_BINARY_DIR}/generated/${SHADER_TARGET}"
    )

    set(
        SPIRV_DIRECTORY
        "${GENERATED_ROOT}/spirv"
    )

    set(
        CPP_DIRECTORY
        "${GENERATED_ROOT}/cpp"
    )

    file(MAKE_DIRECTORY "${SPIRV_DIRECTORY}")
    file(MAKE_DIRECTORY "${CPP_DIRECTORY}")

    # =========================================================================
    # glslang common arguments
    # =========================================================================

    set(
        GLSLANG_COMMON_ARGS
        "--target-env"
        "${SHADER_TARGET_ENV}"
    )

    #
    # -G = SPIR-V using OpenGL semantics
    # -V = SPIR-V using Vulkan semantics
    #

    if(SHADER_TARGET_ENV MATCHES "^opengl")
        list(APPEND GLSLANG_COMMON_ARGS "-G")
    else()
        list(APPEND GLSLANG_COMMON_ARGS "-V")
    endif()

    foreach(INCLUDE_DIR IN LISTS SHADER_INCLUDE_DIRECTORIES)
        list(APPEND
            GLSLANG_COMMON_ARGS
            "-I${INCLUDE_DIR}"
        )
    endforeach()

    foreach(DEFINITION IN LISTS SHADER_DEFINITIONS)
        list(APPEND
            GLSLANG_COMMON_ARGS
            "-D${DEFINITION}"
        )
    endforeach()

    # =========================================================================
    # Generated file lists
    # =========================================================================

    set(GENERATED_CPP_FILES)
    set(GENERATED_HPP_FILES)
    set(SPIRV_FILES)

    set(REGISTRY_DECLARATIONS "")
    set(REGISTRY_ENTRIES "")

    # =========================================================================
    # Process shaders
    # =========================================================================

    foreach(SHADER_SOURCE IN LISTS SHADER_SOURCES)

        # ---------------------------------------------------------------------
        # Relative shader path
        # ---------------------------------------------------------------------

        file(
            RELATIVE_PATH
            SHADER_RELATIVE_PATH
            "${SHADER_DIRECTORY}"
            "${SHADER_SOURCE}"
        )

        # ---------------------------------------------------------------------
        # Determine shader stage
        # ---------------------------------------------------------------------

        if(SHADER_RELATIVE_PATH MATCHES "\\.vert(\\.glsl)?$")
            set(SHADER_STAGE "vert")

        elseif(SHADER_RELATIVE_PATH MATCHES "\\.frag(\\.glsl)?$")
            set(SHADER_STAGE "frag")

        elseif(SHADER_RELATIVE_PATH MATCHES "\\.comp(\\.glsl)?$")
            set(SHADER_STAGE "comp")

        elseif(SHADER_RELATIVE_PATH MATCHES "\\.geom(\\.glsl)?$")
            set(SHADER_STAGE "geom")

        elseif(SHADER_RELATIVE_PATH MATCHES "\\.tesc(\\.glsl)?$")
            set(SHADER_STAGE "tesc")

        elseif(SHADER_RELATIVE_PATH MATCHES "\\.tese(\\.glsl)?$")
            set(SHADER_STAGE "tese")

        else()
            message(FATAL_ERROR
                "Unable to determine shader stage for: "
                "${SHADER_RELATIVE_PATH}"
            )
        endif()

        # ---------------------------------------------------------------------
        # Generate safe C++ identifier
        #
        # post/tonemap.frag
        #
        # becomes:
        #
        # post_tonemap_frag
        # ---------------------------------------------------------------------

        string(
            REGEX REPLACE
            "[^A-Za-z0-9_]"
            "_"
            SHADER_IDENTIFIER
            "${SHADER_RELATIVE_PATH}"
        )

        #
        # Make sure an identifier never starts with a number.
        #

        if(SHADER_IDENTIFIER MATCHES "^[0-9]")
            set(
                SHADER_IDENTIFIER
                "_${SHADER_IDENTIFIER}"
            )
        endif()

        # ---------------------------------------------------------------------
        # Output paths
        # ---------------------------------------------------------------------

        set(
            SPIRV_FILE
            "${SPIRV_DIRECTORY}/${SHADER_IDENTIFIER}.spv"
        )

        set(
            CPP_FILE
            "${CPP_DIRECTORY}/${SHADER_IDENTIFIER}.cpp"
        )

        set(
            HPP_FILE
            "${CPP_DIRECTORY}/${SHADER_IDENTIFIER}.hpp"
        )

        # =====================================================================
        # GLSL -> SPIR-V
        # =====================================================================

        add_custom_command(
            OUTPUT
                "${SPIRV_FILE}"

            COMMAND
                "${GLSLANG_VALIDATOR}"
                ${GLSLANG_COMMON_ARGS}
                -S "${SHADER_STAGE}"
                -o "${SPIRV_FILE}"
                "${SHADER_SOURCE}"

            DEPENDS
                "${SHADER_SOURCE}"

            COMMENT
                "Compiling GLSL ${SHADER_RELATIVE_PATH} -> SPIR-V"

            VERBATIM
            COMMAND_EXPAND_LISTS
        )

        # =====================================================================
        # SPIR-V + GLSL source -> generated C++
        # =====================================================================

        add_custom_command(
            OUTPUT
                "${CPP_FILE}"
                "${HPP_FILE}"

            COMMAND
                "${CMAKE_COMMAND}"

                "-DSPIRV_FILE=${SPIRV_FILE}"
                "-DSOURCE_FILE=${SHADER_SOURCE}"

                "-DOUTPUT_CPP=${CPP_FILE}"
                "-DOUTPUT_HPP=${HPP_FILE}"

                "-DSHADER_IDENTIFIER=${SHADER_IDENTIFIER}"
                "-DSHADER_PATH=${SHADER_RELATIVE_PATH}"
                "-DSHADER_NAMESPACE=${SHADER_NAMESPACE}"

                -P
                "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/EmbedSPIRV.cmake"

            DEPENDS
                "${SPIRV_FILE}"
                "${SHADER_SOURCE}"
                "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/EmbedSPIRV.cmake"

            COMMENT
                "Embedding shader ${SHADER_RELATIVE_PATH}"

            VERBATIM
        )

        list(
            APPEND
            SPIRV_FILES
            "${SPIRV_FILE}"
        )

        list(
            APPEND
            GENERATED_CPP_FILES
            "${CPP_FILE}"
        )

        list(
            APPEND
            GENERATED_HPP_FILES
            "${HPP_FILE}"
        )

        # =====================================================================
        # Registry information
        # =====================================================================

        string(
            APPEND
            REGISTRY_DECLARATIONS
            "#include \"${SHADER_IDENTIFIER}.hpp\"\n"
        )

        string(
            APPEND
            REGISTRY_ENTRIES
            "    &${SHADER_IDENTIFIER},\n"
        )

    endforeach()

    # =========================================================================
    # Registry
    # =========================================================================

    list(
        LENGTH
        SHADER_SOURCES
        SHADER_COUNT
    )

    set(
        REGISTRY_HPP
        "${CPP_DIRECTORY}/ShaderRegistry.hpp"
    )

    set(
        REGISTRY_CPP
        "${CPP_DIRECTORY}/ShaderRegistry.cpp"
    )

    # =========================================================================
    # ShaderRegistry.hpp
    # =========================================================================

    file(
        GENERATE
        OUTPUT
            "${REGISTRY_HPP}"

        CONTENT
"#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace ${SHADER_NAMESPACE}
{

struct ShaderBinary
{
    std::string_view path;

    std::span<const std::uint32_t> spirv;

    std::string_view source;

    [[nodiscard]]
    constexpr std::size_t wordCount() const noexcept
    {
        return spirv.size();
    }

    [[nodiscard]]
    constexpr std::size_t byteSize() const noexcept
    {
        return spirv.size_bytes();
    }

    [[nodiscard]]
    constexpr const std::uint32_t* data() const noexcept
    {
        return spirv.data();
    }

    [[nodiscard]]
    constexpr bool empty() const noexcept
    {
        return spirv.empty();
    }
};

[[nodiscard]]
const ShaderBinary& GetShader(std::string_view name);

[[nodiscard]]
std::span<const ShaderBinary* const> GetShaders();

}
"
    )

    # =========================================================================
    # ShaderRegistry.cpp
    # =========================================================================

    file(
        GENERATE
        OUTPUT
            "${REGISTRY_CPP}"

        CONTENT
"#include \"ShaderRegistry.hpp\"

${REGISTRY_DECLARATIONS}

#include <array>
#include <stdexcept>
#include <string_view>

namespace ${SHADER_NAMESPACE}
{

namespace
{

const std::array<const ShaderBinary*, ${SHADER_COUNT}> Shaders =
{
${REGISTRY_ENTRIES}
};

}

const ShaderBinary& GetShader(std::string_view name)
{
    for (const ShaderBinary* shader : Shaders)
    {
        if (shader->path == name)
        {
            return *shader;
        }
    }

    throw std::runtime_error(
        \"Shader not found: \" + std::string(name)
    );
}

std::span<const ShaderBinary* const> GetShaders()
{
    return Shaders;
}

}
"
    )

    # =========================================================================
    # Library target
    # =========================================================================

    add_library(
        ${SHADER_TARGET}
        STATIC

        ${GENERATED_CPP_FILES}
        "${REGISTRY_CPP}"
    )

    target_sources(
        ${SHADER_TARGET}

        PUBLIC
            FILE_SET HEADERS

            BASE_DIRS
                "${CPP_DIRECTORY}"

            FILES
                ${GENERATED_HPP_FILES}
                "${REGISTRY_HPP}"
    )

    target_include_directories(
        ${SHADER_TARGET}

        PUBLIC
            "${CPP_DIRECTORY}"
    )

    target_compile_features(
        ${SHADER_TARGET}

        PUBLIC
            cxx_std_20
    )

    # =========================================================================
    # Raw SPIR-V target
    #
    # Allows:
    #
    #     cmake --build build --target VISR_OpenGLShaders_SPIRV
    #
    # without compiling the generated C++ library.
    # =========================================================================

    add_custom_target(
        ${SHADER_TARGET}_SPIRV

        DEPENDS
            ${SPIRV_FILES}
    )

    add_dependencies(
        ${SHADER_TARGET}
        ${SHADER_TARGET}_SPIRV
    )

    message(
        STATUS
        "Created shader library '${SHADER_TARGET}' "
        "(${SHADER_COUNT} shaders) "
        "from '${SHADER_DIRECTORY}' "
        "for '${SHADER_TARGET_ENV}'"
    )

endfunction()
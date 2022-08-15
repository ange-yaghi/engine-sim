
if (DISCORD_ENABLED)
    add_library(discord STATIC
        # Source files
        dependencies/discord/Discord.cpp

        # Include files
        dependencies/discord/Discord.h
        dependencies/discord/discord_register.h
        dependencies/discord/discord_rpc.h
    )

    add_library(discord-rpc STATIC IMPORTED)
    set_property(TARGET discord-rpc PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/dependencies/discord/lib/discord-rpc.lib)
    set_property(TARGET discord PROPERTY FOLDER "discord")
endif()

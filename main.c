#include "pumpkin_api.h"

// Plugin metadata
static const char *authors[] = {"Zerorifter"};
static const char *permissions[] = {"slashnv:nv"};

static pumpkin_metadata_t get_meta(void) {
    return (pumpkin_metadata_t) {
        .name = "slashnv",
        .version = "0.1.0",
        .authors = authors,
        .authors_count = sizeof(authors) / sizeof(authors[0]),
        .description = "A simple plugin to add a nightvision command",
        .dependencies_count = 0,
        .permissions = permissions,
        .permissions_count = sizeof(permissions) / sizeof(permissions[0])
    };
}

// Send a message to the command sender
static void send_message(pumpkin_plugin_command_borrow_command_sender_t sender,
                         const char *message) {
    plugin_string_t text;
    plugin_string_set(&text, message);
    pumpkin_plugin_text_own_text_component_t component =
        pumpkin_plugin_text_static_text_component_text(&text);
    pumpkin_plugin_command_method_command_sender_send_message(sender, component);
}

// Handle the nightvision command
static bool handle_command(plugin_own_command_sender_t sender,
                           plugin_own_server_instance_t server,
                           plugin_own_consumed_args_t args,
                           int32_t *ret,
                           plugin_command_error_t *err) {
    (void)server;
    (void)args;
    (void)err;
    // Ensure the sender is a player!
    pumpkin_plugin_command_borrow_command_sender_t borrowed_sender =
        pumpkin_plugin_command_borrow_command_sender(sender);
    pumpkin_plugin_command_own_player_t player;
    if (!pumpkin_plugin_command_method_command_sender_as_player(
            borrowed_sender, &player)) {
        *ret = 0;
        return true;
    }
    
    // Defining the effect
    pumpkin_plugin_player_status_effect_instance_t effect = {
        .effect_type = PUMPKIN_PLUGIN_STATUS_EFFECT_STATUS_EFFECT_TYPE_NIGHT_VISION,
        .duration = -1,
        .amplifier = 0,
        .ambient = false,
        .show_particles = false,
        .show_icon = false
    };
    pumpkin_plugin_player_borrow_player_t borrowed_player =
    pumpkin_plugin_player_borrow_player(player);

    bool has_night_vision = pumpkin_plugin_player_method_player_has_effect(
        borrowed_player,
        PUMPKIN_PLUGIN_STATUS_EFFECT_STATUS_EFFECT_TYPE_NIGHT_VISION);
    if (!has_night_vision) {
        pumpkin_plugin_player_method_player_add_effect(pumpkin_plugin_player_borrow_player(player), &effect);
        pumpkin_plugin_player_player_drop_own(player);
    } else {
        pumpkin_plugin_player_method_player_remove_effect(
            borrowed_player,
            PUMPKIN_PLUGIN_STATUS_EFFECT_STATUS_EFFECT_TYPE_NIGHT_VISION);
    }
    *ret = 1;
    return true;
}

// Entry function
// Registers the permission node, command and command aliases
static void on_load(plugin_own_context_t ctx) {
    pumpkin_plugin_permission_permission_t permission = {0};
    plugin_string_set(&permission.node, "slashnv:nv");
    plugin_string_set(&permission.description,
                      "Allows executing the night vision command");
    permission.default_.tag =
        PUMPKIN_PLUGIN_PERMISSION_PERMISSION_DEFAULT_ALLOW;

    plugin_string_t error = {0};
    bool permission_registered =
        pumpkin_plugin_context_method_context_register_permission(
            pumpkin_plugin_context_borrow_context(ctx), &permission, &error);
    if (!permission_registered) {
        pumpkin_plugin_logging_log(PUMPKIN_PLUGIN_LOGGING_LEVEL_ERROR, &error);
        plugin_string_free(&error);
        return;
    }

    plugin_string_t command_names_storage[] = {
        { (uint8_t *)"nightvision", sizeof("nightvision") - 1 },
        { (uint8_t *)"nv", sizeof("nv") - 1 }
    };
    plugin_list_string_t command_names = {
        .ptr = command_names_storage,
        .len = sizeof(command_names_storage) / sizeof(command_names_storage[0])
    };
    plugin_string_t description;
    plugin_string_set(&description, "Gives the player Night Vision");

    pumpkin_plugin_command_own_command_t command =
        pumpkin_plugin_command_constructor_command(&command_names, &description);
    pumpkin_plugin_command_method_command_execute_with_handler_id(
        pumpkin_plugin_command_borrow_command(command),
        (uint32_t)(uintptr_t)handle_command);

    plugin_string_t required_permission;
    plugin_string_set(&required_permission, "slashnv:nv");
    pumpkin_plugin_context_method_context_register_command(
        pumpkin_plugin_context_borrow_context(ctx), command, &required_permission);
}

REGISTER_PUMPKIN_PLUGIN(((pumpkin_plugin_t){
    .get_metadata = get_meta,
    .on_load = on_load
}))

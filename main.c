#include "pumpkin_api.h"

// Plugin metadata
static const char *authors[] = {"Zerorifter"};

static pumpkin_metadata_t get_meta(void) {
    return (pumpkin_metadata_t) {
        .name = "slashnv",
        .version = "0.3.0",
        .authors = authors,
        .authors_count = sizeof(authors) / sizeof(authors[0]),
        .description = "A simple plugin to add a nightvision command",
        .dependencies_count = 0,
    };
}

// NBT That helps persist nightvision
// We use it when checking wether to give a player nightvision or not
#define PLUGIN_STR(s) { .ptr = (uint8_t *)(s), .len = sizeof(s) - 1 }

static plugin_string_t NV_NAMESPACE = PLUGIN_STR("slashNV");
static plugin_string_t NV_KEY       = PLUGIN_STR("hasnv");

static pumpkin_plugin_common_nbt_tag_t nv_tag = {
    .tag = PUMPKIN_PLUGIN_COMMON_NBT_TAG_BYTE,
    .val.byte = 1,
};

static pumpkin_plugin_world_nbt_tree_t nv_marker = {
    .root = 0,
    .tags = { .ptr = &nv_tag, .len = 1 },
};

// Defining the NightVision effect we're using
static pumpkin_plugin_player_status_effect_instance_t NV_EFFECT = {
    .effect_type    = PUMPKIN_PLUGIN_STATUS_EFFECT_STATUS_EFFECT_TYPE_NIGHT_VISION,
    .duration       = -1,   // infinite
    .amplifier      = 0,
    .ambient        = false,
    .show_particles = false,
    .show_icon      = false,
};

static bool has_night_vision_effect(pumpkin_plugin_player_borrow_player_t borrowed_player) {
    return pumpkin_plugin_player_method_player_has_effect(borrowed_player, PUMPKIN_PLUGIN_STATUS_EFFECT_STATUS_EFFECT_TYPE_NIGHT_VISION);
}

static bool has_night_vision_tag(pumpkin_plugin_world_borrow_entity_t borrowed_player_entity) {
    return pumpkin_plugin_world_method_entity_has_custom_data(borrowed_player_entity, &NV_NAMESPACE, &NV_KEY);
}
static void give_night_vision_effect(pumpkin_plugin_player_borrow_player_t borrowed_player) {
    pumpkin_plugin_player_method_player_add_effect(borrowed_player, &NV_EFFECT);
}
static void give_night_vision(pumpkin_plugin_player_borrow_player_t borrowed_player, pumpkin_plugin_world_borrow_entity_t borrowed_player_entity) {
    give_night_vision_effect(borrowed_player);
    pumpkin_plugin_world_method_entity_set_custom_data(borrowed_player_entity, &NV_NAMESPACE, &NV_KEY, &nv_marker);
}

static void remove_night_vision(pumpkin_plugin_player_borrow_player_t borrowed_player, pumpkin_plugin_world_borrow_entity_t borrowed_player_entity) {
    pumpkin_plugin_player_method_player_remove_effect(borrowed_player, PUMPKIN_PLUGIN_STATUS_EFFECT_STATUS_EFFECT_TYPE_NIGHT_VISION);
    pumpkin_plugin_world_method_entity_remove_custom_data(borrowed_player_entity, &NV_NAMESPACE, &NV_KEY);
}

static void reapply_night_vision(plugin_own_server_instance_t server) {
    pumpkin_plugin_server_borrow_server_t borrowed_server =
        pumpkin_plugin_server_borrow_server(server);
    pumpkin_plugin_server_list_own_player_t players;
    pumpkin_plugin_server_method_server_get_all_players(borrowed_server, &players);
    
    for (size_t i = 0; i < players.len; i++) {
        pumpkin_plugin_player_borrow_player_t borrowed_player =
            pumpkin_plugin_player_borrow_player(players.ptr[i]);
        pumpkin_plugin_player_own_entity_t player_entity = 
            pumpkin_plugin_player_method_player_as_entity(borrowed_player);
        pumpkin_plugin_world_borrow_entity_t borrowed_player_entity = 
            pumpkin_plugin_world_borrow_entity(player_entity);
        if (has_night_vision_tag(borrowed_player_entity) && !has_night_vision_effect(borrowed_player)) {
            give_night_vision_effect(borrowed_player);
        }
        pumpkin_plugin_world_entity_drop_own(player_entity);
        pumpkin_plugin_player_player_drop_own(players.ptr[i]);
    }
    pumpkin_plugin_server_list_own_player_free(&players);
    pumpkin_plugin_server_server_drop_own(server);
}

// Handle the nightvision command
static bool handle_command(plugin_own_command_sender_t sender,
                           plugin_own_server_instance_t server,
                           plugin_own_consumed_args_t args,
                           int32_t *ret,
                           plugin_command_error_t *err) {
    pumpkin_plugin_server_server_drop_own(server);
    pumpkin_plugin_command_consumed_args_drop_own(args);
    // Ensure the sender is a player!
    pumpkin_plugin_command_borrow_command_sender_t borrowed_sender =
        pumpkin_plugin_command_borrow_command_sender(sender);
    pumpkin_plugin_command_own_player_t player;
    if (!pumpkin_plugin_command_method_command_sender_as_player(
            borrowed_sender, &player)) {
        pumpkin_plugin_command_command_sender_drop_own(sender);        
        *ret = 0;
        return true;
    }
    
    pumpkin_plugin_player_borrow_player_t borrowed_player = 
        pumpkin_plugin_player_borrow_player(player);
    pumpkin_plugin_player_own_entity_t player_entity = 
        pumpkin_plugin_player_method_player_as_entity(borrowed_player);
    pumpkin_plugin_world_borrow_entity_t borrowed_player_entity = 
        pumpkin_plugin_world_borrow_entity(player_entity);

    // Toggle effect logic
    if (!has_night_vision_tag(borrowed_player_entity)) {
        give_night_vision(borrowed_player, borrowed_player_entity);
    } 
    else {
        remove_night_vision(borrowed_player, borrowed_player_entity);
    }    
    pumpkin_plugin_world_entity_drop_own(player_entity);
    pumpkin_plugin_player_player_drop_own(player);
    pumpkin_plugin_command_command_sender_drop_own(sender);
    *ret = 1;
    return true;
}



// Entry function
// Registers the permission node, command and command aliases
static void on_load(plugin_own_context_t ctx) {
    pumpkin_plugin_permission_permission_t permission = {0};
    plugin_string_set(&permission.node, "slashnv:nv");
    plugin_string_set(&permission.description,
                      "Allows executing the /nv command");
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
    plugin_string_set(&description, "Grants Night Vision to the player");

    pumpkin_plugin_command_own_command_t command =
        pumpkin_plugin_command_constructor_command(&command_names, &description);
    pumpkin_plugin_command_method_command_execute_with_handler_id(
        pumpkin_plugin_command_borrow_command(command),
        (uint32_t)(uintptr_t)handle_command);

    plugin_string_t required_permission;
    plugin_string_set(&required_permission, "slashnv:nv");
    pumpkin_plugin_context_method_context_register_command(
        pumpkin_plugin_context_borrow_context(ctx), command,
        &required_permission);

    // Reapply every second (20 ticks).
    pumpkin_scheduler_schedule_repeating_task(reapply_night_vision, 0, 20);
}

REGISTER_PUMPKIN_PLUGIN(((pumpkin_plugin_t){
    .get_metadata = get_meta,
    .on_load = on_load
}))

[![DDraceNetwork](docs/assets/TClient_Logo_Horizontal.svg)](https://tclient.app) 

[![Build](https://github.com/sjrc6/TaterClient-ddnet/workflows/Build/badge.svg)](https://github.com/sjrc6/TaterClient-ddnet/actions/workflows/build.yaml)

### Taters custom ddnet client with some modifications

Not guarenteed to be bug free, but I will try to fix them.

If ddnet devs are reading this and want to steal my changes please feel free.

Thanks to tela for the logo design, and solly for svg <3

### Links

[Discord](https://discord.gg/BgPSapKRkZ)
[Website](https://tclient.app)

### Installation

* Download the latest [release](https://github.com/sjrc6/TaterClient-ddnet/releases)
* Download a [nightly (dev/unstable) build](https://github.com/sjrc6/TaterClient-ddnet/actions/workflows/fast-build.yml?query=branch%3Amaster)
* [Clone](https://docs.github.com/en/repositories/creating-and-managing-repositories/cloning-a-repository) this repo and build using the [guide from DDNet](https://github.com/ddnet/ddnet?tab=readme-ov-file#cloning)

### Conditional Tutorial

There are certain vars you can write which get substituted before execution, these are wrapped in `{` and `)`  
These are limited to not let you do things like adding extra control capabilities  
Here is a list of variables which are available:  
`game_mode`, `game_mode_pvp`, `game_mode_race`, `eye_wheel_allowed`, `zoom_allowed`, `dummy_allowed`, `dummy_connected`, `rcon_authed`, `map`, `server_ip`, `players_connected`, `players_cap`, `server_name`, `community`, `location`  

| Name | Args | Description |
| --- | --- | --- |
| `ifeq` | `s[a] s[b] r[command]` | Comapre 2 values, if equal run the command |
| `ifneq` | `s[a] s[b] r[command]` | Comapre 2 values, if not equal run the command |
| `ifreq` | `s[a] s[b] r[command]` | Comapre 2 values, if a matches the regex b run the command |
| `ifrneq` | `s[a] s[b] r[command]` | Comapre 2 values, if a doesnt match the regex b run the command |

(Note the regex engine is [Remimu](https://github.com/wareya/Remimu))

With the commands listed above and the substitutions you can create simple comparisons, here is a few examples.

```
echo There are {players_connected} players connected
I chat/client: — There are 7 players connected
```

```
ifreq {server_ip} "^(49\.13\.73\.199|188\.245\.101\.41|188\.245\.66\.93|20\.215\.41\.104):\d+$" say /login AWB CODE
ifeq {community} kog say /login KOG CODE
```

```
ifeq "$(community) $(game_mode)" "ddnet DDraceNetwork" rcon_login USER PASS
```

```
exec scripts/sewerslide_off.cfg
ifeq {game_mode_pvp} 1 exec scripts/sewerslide_on.cfg
ifeq {map} Linear exec scripts/sewerslide_on.cfg
ifreq {map} "^.*?Copy Love Box.*?$" exec scripts/sewerslide_on.cfg
ifeq {game_mode} 0XF exec scripts/sewerslide_on.cfg
```

There is also a `return` which lets you early return from scripts.
* Have "elses" without spamming `ifeq`
* Have early returns while still remaining compatible with DDNet

Here is an example which I use for dummy connecting
```
ifneq {dummy_connected} 0 return
dummy_connect
exec "scripts/dummy/reset.cfg"
```

### Settings Page

![image](https://github.com/user-attachments/assets/a6ccb206-9fed-48be-a2d2-8fc50a6be882)
![image](https://github.com/user-attachments/assets/9251509a-d852-41ac-bf6b-9a610db08945)
![image](https://github.com/user-attachments/assets/47dab977-1311-4963-a11a-81b78005b12b)
![image](https://github.com/user-attachments/assets/29bddfd9-fcf1-420c-b7e0-958493051a3c)
![image](https://github.com/user-attachments/assets/efe3528f-a962-4dc0-aa8c-9ca963c246e5)
![image](https://github.com/user-attachments/assets/9f15023d-2a27-44ee-8157-e76da53c875a)

![image](https://user-images.githubusercontent.com/22122579/182528700-4c8238c3-836e-49c3-9996-68025e7f5d58.png)

### Features

```
tc_run_on_join_console
tc_run_on_join_delay
tc_nameplate_ping_circle
tc_hammer_rotates_with_cursor
tc_freeze_update_fix
tc_show_center
tc_skin_name
tc_color_freeze
tc_freeze_stars
tc_white_feet
tc_white_feet_skin
tc_mini_debug
tc_last_notify
tc_last_notify_text
tc_last_notify_color
tc_cursor_in_spec
tc_render_nameplate_spec
tc_fast_input
tc_fast_input_others
tc_improve_mouse_precision
tc_frozen_tees_hud
tc_frozen_tees_text
tc_frozen_tees_hud_skins
tc_frozen_tees_size
tc_frozen_tees_max_rows
tc_frozen_tees_only_inteam
tc_remove_anti
tc_remove_anti_ticks
tc_remove_anti_delay_ticks
tc_unpred_others_in_freeze
tc_pred_margin_in_freeze
tc_pred_margin_in_freeze_amount
tc_show_others_ghosts
tc_swap_ghosts
tc_hide_frozen_ghosts
tc_pred_ghosts_alpha
tc_unpred_ghosts_alpha
tc_render_ghost_as_circle
tc_outline
tc_outline_in_entities
tc_outline_freeze
tc_outline_unfreeze
tc_outline_tele
tc_outline_solid
tc_outline_width
tc_outline_alpha
tc_outline_alpha_solid
tc_outline_color_solid
tc_outline_color_freeze
tc_outline_color_tele
tc_outline_color_unfreeze
tc_player_indicator
tc_player_indicator_freeze
tc_indicator_alive
tc_indicator_freeze
tc_indicator_dead
tc_indicator_offset
tc_indicator_offset_max
tc_indicator_variable_distance
tc_indicator_variable_max_distance
tc_indicator_radius
tc_indicator_opacity
tc_indicator_inteam
tc_indicator_tees
tc_profile_skin
tc_profile_name
tc_profile_clan
tc_profile_flag
tc_profile_colors
tc_profile_emote
tc_auto_verify
tc_rainbow
tc_rainbow_others
tc_rainbow_mode
tc_reset_bindwheel_mouse
add_profile
add_bindwheel
remove_bindwheel
delete_all_bindwheel_binds
+bindwheel_execute_hover
+bindwheel
tc_regex_chat_ignore
tc_color_freeze_darken
tc_color_freeze_feet
tc_spec_menu_ID
tc_limit_mouse_to_screen
```

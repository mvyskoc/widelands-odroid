-- This script defines a GUI style for Widelands. At the moment, we only
-- support the default template.
-- So far, only background textures and colors can be defined, and they all have
-- the format { image = filename, color = {r, g, b } }.

dirname = path.dirname(__file__)

-- Some common elements for reuse
local fs_button = dirname .. "fsmenu/button.png"
local wui_button = dirname .. "wui/button.png"

local fs_blue =  {0, 31, 40}
local fs_green =  {10, 50, 0}
local fs_brown =  {45, 34, 18}

local wui_light =  {85, 63, 35}
local wui_green =  {3, 15, 0}
local wui_brown =  {32, 19, 8}

local fs_font_color = {255, 255, 0}
local fs_font_face = "sans"
local fs_font_size = 14

local wui_font_color = {255, 255, 0}
local wui_font_face = "sans"
local wui_font_size = 12

-- These are the style definitions to be returned.
-- Note: you have to keep all the keys intact, or Widelands will not be happy.
return {
   -- Button backgrounds
   buttons = {
      -- Buttons used in Fullscreen menus
      fsmenu = {
         -- Main menu ("Single Player", "Watch Replay", ...)
         menu = { image = fs_button, color = fs_blue },
         -- Primary user selection ("OK", ...)
         primary = { image = fs_button, color = fs_green },
         -- Secondary user selection ("Cancel", "Delete", selection buttons, ...)
         secondary = { image = fs_button, color = fs_brown },
      },
      -- Buttons used in-game and in the editor
      wui = {
         -- Main menu ("Exit Game"), Building Windows, selection buttons, ...
         menu = { image = wui_button, color = wui_light },
         -- Primary user selection ("OK", attack, ...)
         primary = { image = wui_button, color = wui_green },
         -- Secondary user selection ("Cancel", "Delete", ...)
         secondary = { image = wui_button, color = wui_brown },
         -- Building buttons on fieldaction and building statistics need to be
         -- transparent in order to match the background of the tab panel.
         building_stats = { image = "", color = {0, 0, 0} },
      }
   },
   -- Slider cursors (Sound control, attack, statistics, ...)
   sliders = {
      fsmenu = {
         menu = { image = fs_button, color = fs_blue },
      },
      wui = {
         -- Sound Options, Statistics
         light = { image = wui_button, color = wui_brown },
         -- Fieldaction (attack)
         dark = { image = wui_button, color = wui_green },
      }
   },
   -- Background for tab panels
   tabpanels = {
      fsmenu = {
         -- Options, About, ... this comes with a hard-coded border too
         menu = { image = "", color = {5, 5, 5} },
      },
      wui = {
         -- Most in-game and in-editor tabs. Building windows, Editor tools,
         -- Encyclopedia, ...
         light = { image = "", color = {0, 0, 0} },
         -- Building buttons in Fieldaction and Building Statistics need a dark
         -- background, otherwise the icons will be hard to see.
         dark = { image = wui_button, color = wui_brown },
      }
   },
   -- Used both for one-line and multiline edit boxes
   editboxes = {
      fsmenu = {
         menu = { image = fs_button, color = fs_green },
      },
      wui = {
         menu = { image = wui_button, color = wui_brown },
      }
   },
   -- Background for dropdown menus
   dropdowns = {
      fsmenu = {
         menu = { image = fs_button, color = fs_brown },
      },
      wui = {
         menu = { image = wui_button, color = wui_brown },
      }
   },
   -- Scrollbar buttons, table headers etc.
   scrollbars = {
      fsmenu = {
         menu = { image = fs_button, color = fs_blue },
      },
      wui = {
         menu = { image = wui_button, color = wui_brown },
      }
   },

   font_styles = {
      -- Font sizes and colors
      --[[
         required: face, color, size;
         optional bools: bold, italic, underline, shadow
      ]]
      -- Buttons
      button = {
         color = fs_font_color,
         face = fs_font_face,
         size = fs_font_size,
         bold = true,
         shadow = true
      },
      -- Intro screen
      fsmenu_intro = {
         color = { 192, 192, 128 },
         face = fs_font_face,
         size = fs_font_size,
         bold = true,
         shadow = true
      },
      -- Game and Map info panels
      fsmenu_info_panel_heading = {
         color = { 255, 255, 0 },
         face = fs_font_face,
         size = fs_font_size,
         bold = true,
         shadow = true
      },
      fsmenu_info_panel_paragraph = {
         color = { 209, 209, 209 },
         face = fs_font_face,
         size = fs_font_size,
         shadow = true
      },
      wui_info_panel_heading = {
         color = { 209, 209, 209 },
         face = fs_font_face,
         size = fs_font_size,
         bold = true,
      },
      wui_info_panel_paragraph = {
         color = { 255, 255, 0 },
         face = fs_font_face,
         size = fs_font_size,
      },
      -- Messages
      wui_message_heading = {
         color = { 209, 209, 209 },
         face = wui_font_face,
         size = 18,
         bold = true,
      },
      wui_message_paragraph = {
         color = { 255, 255, 0 },
         face = wui_font_face,
         size = wui_font_size,
      },
      tooltip = {
         color = fs_font_color,
         face = fs_font_face,
         size = fs_font_size,
         bold = true,
      },
      wui_waresinfo = {
         color = wui_font_color,
         face = "condensed",
         size = 10,
      },
      -- Basic chat message text color
      chat_message = {
         color = wui_font_color,
         face = "serif",
         size = fs_font_size,
         shadow = true,
      },
      -- Basic chat message text color
      chat_timestamp = {
         color = { 51, 255, 51 },
         face = "serif",
         size = 9,
         shadow = true,
      },
      -- Chat for private messages
      chat_whisper = {
         color = { 238, 238, 238 },
         face = "serif",
         size = fs_font_size,
         italic = true,
         shadow = true,
      },
      -- Chat playername highlight
      chat_playername = {
         color = { 255, 255, 255 },
         face = "serif",
         size = fs_font_size,
         bold = true,
         underline = true,
         shadow = true,
      },
      -- Chat log messages color. Also doubles as spectator playercolor for chat messages.
      chat_server = {
         color = { 221, 221, 221 },
         face = "serif",
         size = fs_font_size,
         bold = true,
         shadow = true,
      },
      fsmenu_gametip = {
         color = { 0, 0, 0 },
         face = "serif",
         size = 16,
      },
   },

   -- NOCOM clean this up and remove
   fonts = {
      sizes = {
         title = 22,    -- Big titles
         normal = 14,   -- Default UI color
         slider = 11,   -- Slider font size
         minimum = 6,   -- When autoresizing text to fit, don't go below this size
      },
      colors = {
         foreground = fs_font_color, -- Main UI color
         disabled = {127, 127, 127}, -- Disabled interactive UI elements
         warning = {255, 22, 22},    -- For highlighting warnings
         progresswindow_text = { 128, 128, 255 },    -- FS Progress bar text
         progresswindow_background = { 64, 64, 0 },  -- FS Progress bar background
         progress_bright = {255, 250, 170},          -- Progress bar text
         progress_construction = {163, 144, 19}, -- Construction/Dismantle site progress
         productivity_low = {187, 0, 0},         -- Low building productivity
         productivity_medium = {255, 225, 30},   -- Medium building productivity
         productivity_high = {0, 187, 0},        -- High building productivity
         plot_axis_line = { 0, 0, 0 },       -- Statistics plot
         plot_zero_line = { 255, 255, 255 }, -- Statistics plot
         plot_xtick = { 255, 0, 0 },         -- Statistics plot
         plot_yscale_label = { 60, 125, 0 }, -- Statistics plot
         plot_min_value = { 125, 0, 0 },     -- Statistics plot
         game_setup_headings = { 0, 255, 0 },     -- Internet lobby and launch game
         game_setup_mapname = { 255, 255, 127 },  -- Internet lobby and launch game
         intro = { 192, 192, 128 }
      }
   }
}

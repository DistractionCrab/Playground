# Game Engine Playground
This is a work-in-progress project intended to be a portfolio of what I can do with Unreal Engine and Unity. This is a cross-disciplinary projects with me developing not only the code for a game, but as well as assets, engineering, design, feature implementation and art. This project has been in development for approximately three weeks at this point.

Part of this project has been split into various plugins to improve development across this testbed/portfolio and more concrete projects I am working. Here is a list of plugins available:

- [General Tools](https://github.com/DistractionCrab/CrabToolsUE5/)

## Progress Videos:

- [Month 1](https://youtu.be/Bh_T2ypWLW0)


# General Completed TAsks:
- Base Character State Machine design
- Dynamical realistic water materials.
- Dynamic perspective API
- Interactive objects system and API
- Chasing AI
- Simple general Spellcasting System
- Faction System
- Spell Casting System


# Started
- Melee Combo System (2 chain combos done, blender animations need work).
- Main UI for inventory, stats, and spells.
- Village/Town design
- Character attack animation system.
- Develop dynamical grass system based off of Ghost of Tsushima's bezier approach.
- - Does animate, need to work on parameterizations for individual blades.

# Tasks
- Develop custom gravitational orientations for walking on walls.
- Make the chasing AI attack in various ways.
- Object Pooling system for projectiles
- Update BTT to better manage patrol point index: Current increments the index regardless of success. Not a big problem, but does cause some oddities when noticing the player

# Engineering Design and Approach

The approach for designing this "game" is an event based driven API system that focuses on exposing methods for registering
callbacks in C++ or in Blueprint Event Dispatchers. At this point, of the multiple objects I've created almost none heavily utilize any Ticking calls and focus primarily on events for how objects communicate and update state. A primary example of this is the
design for my [Main Character](https://github.com/DistractionCrab/Playground/blob/main/Playground/Source/Playground/PlaygroundCharacter.h); This character utilizes an event based, procedural state machine system that can be extended to 
provide increased functionality as needed. All of the controls for this character is based of Input events (with the exception of 
when falling starts, which does need to be checked regularly). This class is not abstract, but is designed in such a way, that it can be utilized across multiple different types of characters, regardless of how their animation logic is implemented.

The State Machines register callbacks for when state changes happen, and Enums for communicating what state it is in and what actions are being performed whilst in said states. This provides enough information to any Animation Blueprints on how to transition throughout complex situations. This can be seen in the ABP_PlaygroundCharacter blueprint which shows off a minimal set of logic for managing locomotion, airborne states, and spellcasting (more to come in the future).

# UI Design and Engineering

As with any game, a UI needs to be made, and to facilitate an easy way to update UI's with little querying, I've created an "IntVariable" c++ actor component (Will make more for floats and others as needed). These act as an autonomous variable that can alert any interested parties to their change. This is primarily useful for health bars, which to update need to know exactly when health changes. I chose to design this actor component instead of just utilizing event dispatchers for the following reasons:

- More general: When designing RPG systems later on, it will be useful to capture and overwrite values without the need for creating complex storage/organization systems.
- Actor components are useful for autonomously updating things on ticks or other methods when needed.
- Components are guaranteed when attached to an actor; In contrast when using an object pointer, it can be invalid and may need to be initialized (helps lower the chance of dumb mistakes later on).
- I don't need to create Event dispatchers for every class, and since Event Dispatchers cannot be used from Blueprint Interfaces, I can use a return value on such interfaces to communicate a bit more. 

In terms of design, I am still early in designing how things will look; I am focuses on a modular approach having many different pieces act autonomously without any knowledge of where they may lie in a hierarchy (See Health Bar widgets for an example).

# Level Design

Part of this project is also to demonstrate my ability to design levels and environments. I'm still in the early stages of making a 
primary map to walk through, but you can search through what I've started by downloading this project and going to the Main level.



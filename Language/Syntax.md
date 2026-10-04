# VISR Syntax

VISR is a declarative language for describing **spatial user
interfaces**.

Unlike a traditional desktop, VISR does not assume that every
application owns a window. Applications expose **state** and
**actions**, and VISR decides how those capabilities are composed into a
shared interface.

``` text
Apps → State + Actions → VISR → Spatial Interface
```

## Example

``` xml
<surface shape="cylinder" anchor="left-forearm" radius="80cm" arc="60deg">
    <column>

        <group name="Status">
            <text state="Weather/Temperature" />
            <text state="Calendar/NextEvent/Title" />
        </group>

        <group name="Media">
            <text state="Music/CurrentTrack/Title" />

            <slider
                state="Music/PlaybackPosition"
                action="Music/Seek"
            />

            <button action="Music/PlayPause">
                Play / Pause
            </button>
        </group>

    </column>
</surface>
```

This is **one interface**, not separate Weather, Calendar, and Music
windows.

## Surfaces

`<surface>` describes **where UI exists in physical space** and the
shape it occupies.

``` xml
<surface shape="plane" anchor="waist" width="2m" height="1m">
    ...
</surface>
```

## Layout

Layout elements arrange their children.

``` xml
<row>...</row>
<column>...</column>
<stack>...</stack>
```

They describe organization, not application boundaries.

## Groups

`<group>` is a **semantic grouping**.

``` xml
<group name="Driving">
    <text state="Navigation/ETA" />
    <text state="Weather/Temperature" />
    <button action="Music/Next">Next</button>
</group>
```

Groups do not render anything themselves. They have no background,
geometry, or appearance.

They simply tell VISR that their contents conceptually belong together.
This allows a user or layout manager to understand and reorganize the
interface later.

## State

UI elements can display state exposed by applications.

``` xml
<text state="Weather/Temperature" />
<progress state="System/Focus/Progress" />
<slider state="Music/Volume" />
```

`state` describes the value an element reads or displays.

## Actions

Interactive elements can invoke actions exposed by applications.

``` xml
<button action="Music/PlayPause">
    Play / Pause
</button>
```

Controls may use both state and actions.

``` xml
<slider
    state="Music/Volume"
    action="Music/SetVolume"
/>
```

`state` describes what the control displays.

`action` describes what happens when the user interacts with it.

## Panels

Built-in elements such as `<text>`, `<button>`, and `<slider>` handle
common UI.

For custom rendering or interaction, `<panel>` provides a programmable
region backed by JavaScript.

``` xml
<panel
    name="Map"
    script="Navigation/Map.js"
/>
```

The script can use the VISR JavaScript API to read state, invoke
actions, draw raw shapes, create custom visuals, and handle
interactions.

Panels may contain both **2D and 3D content**.

## Philosophy

Traditional desktops generally work like:

``` text
App
 └── Window
      └── UI
```

VISR instead works like:

``` text
Weather ───── State ─────┐
Calendar ──── State ─────┤
Music ─────── State ─────┤
Music ─────── Actions ───┤
Navigation ── State ─────┤
                         ▼
                    VISR Layout
                         │
                         ▼
                Shared Spatial UI
```

**Application boundaries do not need to become visual boundaries.**

A surface can freely combine state and actions from many applications
into one interface, allowing the user, application, or layout manager to
determine how information should appear in the spatial environment.

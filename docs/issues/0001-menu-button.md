---
title: "Decide what Vela's menu button does"
labels: [design, open-question, input]
status: open
---

<!-- Draft of a GitHub issue. File it with:
     gh issue create --title "Decide what Vela's menu button does" \
       --label design --body-file docs/issues/0001-menu-button.md
     then replace this file's status with the issue link. -->

## Context

Vela has three hardware buttons below the screen: back (left), home
(center) and menu (right). On the laptop they are ←, ↓ and →.

Back and home are done: `ui/nav.c` handles them before any screen sees a
key, so they work everywhere. Menu is wired but does nothing.
`keel_nav_set_menu(cb, user)` lets a screen register a handler, and no
screen registers one yet. The bench (`--bench`, "buttons") checks that a
menu press is harmless.

## Question

What should the menu button do?

Options:

1. **Per-screen context sheet.** A list in `title` or `subheading` rows
   slides up over the current screen with actions for what is on it. For
   example: now playing gets shuffle, repeat and go to album; an album gets
   play next and add to playlist; a list row with keyboard focus gets
   actions for that row. Back or menu closes it. Closest to the Zune HD's
   long-press menus, but on a button.
2. **Fixed global action.** Menu always does one thing, for example opens
   now playing, or toggles play/pause when the screen is off.
3. **Hybrid.** Screens with actions get the sheet (1); everywhere else it
   falls back to the global action (2).

## Design-system constraints for any answer

- The sheet is type, not a card. A `raised` area is allowed only if it is
  small, so a full-height sheet stays `base` (black), and the 85% black rule
  still applies.
- An overline above the sheet's title (rule 4); rows at least 72px tall,
  with no dividers.
- The words "close" or "back" are the dismiss affordance, not an icon.
- Motion is ease-out, 200–300 ms, and is off under `--reduced-motion`.

## Done when

- The behavior is decided and written into the README's Input table.
- It is implemented via `keel_nav_set_menu` (or a replacement), with a
  `--screenshot` state for at least one screen that has a menu.
- The bench "buttons" check asserts the new behavior, not just "no crash".

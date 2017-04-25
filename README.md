# dynamic-player-orgs
Facilitates player organizations with online-editable dynamic ranks/permissions/privs within the "PROGENY RPG" codebase

This package implements what should be commonplace in the MUD world: a full engine for clan memberships that can be moderated even when members are offline.  It allows the owners of the clans to change their OWN rank/permission/privilege designations, and deliberately supports the structure of non-traditional clan structures (loose through strict ranks, hidden ranks, multileader or single ownership, et cetera).  There is no fundamental limit on how many of these clans can be created, and there is no need to edit any coded tables to create new/delete old clans.  There is negligible overhead for any MUD integrating this package (the only increase in strain will occur at startup and save times).

From a technical perspective, all the clan data is stored in the game memory as a linked list (of clans), of which each member contains another two linked lists (ranks, and members).  The linked list of clan data is written in plaintext at save time (occurring at the discretion of the programmer), and at startup, read from the same file. 

The following is included and will work out-of-the box, but almost certainly will require tinkering to make work with a separate codebase than “Progeny RPG”.  Included is the following:

• Save/Load routines for clan data

• Admin tools for establishing, deleting, and moderating clans

• Player tools that support dynamic personalization of:

-	Clan Membership 
-	Clan Roles/ranks
-	Default rank permissions
-	Individual permissions that supersede ranks
-	In-game restricted access zones (with “white lists” for allies)
-	In-game restricted bank and storage access (with “white lists” too)
-	Restricted information settings, allowing for spies and hidden agents

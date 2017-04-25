
/*********************************************************************************
I'm now releasing this snippet of code under the GNU 3.0 license, making it free *
to use for any interested parties.  Avail yourself at will. It has been prepped  *
for use within the code portfolio/resume of Oliver Carrillo and represents his   *
personal work (and used within the larger RoM2.4b6 codebase.)*********************
*********************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <uuid/uuid.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
								{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
								}

//clean up the member list to get rid of anything that will break the linked list
void fix_members(MEMBER_DATA *members)
{
	MEMBER_DATA *list, *last = NULL;

	for (list = members; list; list = list->next)
	{
		if (last)
		{
			list->last = last;
		}

		last = list;
	}

	return;
}

//clean up the ranks list to get rid of anything invalid
void fix_ranks(RANK_DATA *ranks)
{
	RANK_DATA *list, *last = NULL;

	for (list = ranks; list; list = list->next)
	{
		if (last)
		{
			list->last = last;
		}

		last = list;
	}

	return;
}

//arrange members according to security ranking
void order_members(MEMBER_DATA *members)
{
	MEMBER_DATA *list, *first, *second, *third, *fourth;
	bool foundswap = FALSE;

	fix_members(members);

	if (!members->last && !members->next) return;

	list = members;

	while (list->next)
	{
		if (list->security_general > list->next->security_general)
		{

			first = list->last;	//NULL
			second = list->next;	//TESTING
			third = list;		//None
			fourth = list->next ? list->next->next : NULL;		//NULL

			if (first)
			{
				first->next = second;
			}

			if (second)
			{
				second->next = third;
				second->last = first;
			}

			third->next = fourth;
			third->last = second;

			foundswap = TRUE;
			break;

		}
		list = list->next;
	}

	while (list->last)
	{
		list = list->last;
	}

	members->org->members = list;

	if (foundswap)
	{
		order_members(members);
	}

	return;
}

//order ranks according to security default
void order_ranks(RANK_DATA *ranks)
{
	RANK_DATA *list, *first, *second, *third, *fourth;
	bool foundswap = FALSE;

	fix_ranks(ranks);

	if (!ranks->last && !ranks->next) return;

	list = ranks;

	while (list->next)
	{
		if (list->default_security_general > list->next->default_security_general)
		{

			first = list->last;	//NULL
			second = list->next;	//TESTING
			third = list;		//None
			fourth = list->next ? list->next->next : NULL;		//NULL

			if (first)
			{
				first->next = second;
			}

			if (second)
			{
				second->next = third;
				second->last = first;
			}

			third->next = fourth;
			third->last = second;

			foundswap = TRUE;
			break;

		}
		list = list->next;
	}

	while (list->last)
	{
		list = list->last;
	}

	ranks->org->ranks = list;

	if (foundswap)
	{
		order_ranks(ranks);
	}

	return;
}

//add a member to an organization
void add_member(ORG_DATA *org, RANK_DATA *rank, CHAR_DATA *ch, char *name)
{
	MEMBER_DATA *member = new_member();

	member->name = str_dup(name);
	member->org = org;
	member->rank = rank;
	member->id = str_dup(ch->uuid);

	member->next = org->members;
	org->members = member;

	order_members(org->members);
	return;
}

//delete a member from an organization
void delete_member(ORG_DATA *org, MEMBER_DATA *member)
{
	MEMBER_DATA *list;

	if (org->members == member)
	{
		if (member->next)
		{
			org->members = member->next;
		}
		else
		{
			org->members = NULL;
		}

		free_member(member);
		return;
	}

	for (list = org->members; list; list = list->next)
	{
		if (list->next == member)
		{
			list->next = member->next;
			free_member(member);
			return;
		}
	}

	return;
}

//delete a rank from the list of org ranks
void delete_rank(RANK_DATA *rank)
{
	RANK_DATA *list;
	MEMBER_DATA *memberlist;

	for (list = rank->org->ranks; list; list = list->next)
	{
		if (list->next == rank)
		{

			for (memberlist = rank->org->members; memberlist; memberlist = memberlist->next)
			{
				if (memberlist->rank == rank)
				{
					memberlist->rank = get_rank_by_name(rank->org, "none");
				}
			}

			list->next = rank->next;
			free_rank(rank);
			return;
		}

		if (list == rank)
		{
			if (list->next)
			{
				list = list->next;
				free_rank(list);
			}
			else
			{
				free_rank(list);
				list->org->ranks = NULL;
			}
			return;
		}
	}

	return;
}

//save the organizations created to the savelist
void save_organizations(void)
{
	FILE *fp;
	ORG_DATA *org;
	RANK_DATA *rank;
	MEMBER_DATA *member;

	if (!org_list)
	{
		return;
	}

	if ((fp = fopen(ORG_DATA_FILE, "w")) == NULL)
	{
		bugf("%s unaccessible.  Org saving failed.", ORG_DATA_FILE);
		perror(ORG_DATA_FILE);
		return;
	}

	org = org_list;

	while (org)
	{
		fprintf(fp, "Beginorg\n");
		fprintf(fp, "Name %s~\n", org->name);
		fprintf(fp, "Type %d\n", org->type);
		fprintf(fp, "SecLeader %d\n", org->security_level_leader);
		fprintf(fp, "SecBank %d\n", org->security_level_bank);
		fprintf(fp, "SecRemove %d\n", org->security_level_discharge);
		fprintf(fp, "SecAdmit %d\n", org->security_level_admit);
		fprintf(fp, "SecPassMain %d\n", org->security_level_passage_main);
		fprintf(fp, "SecPassVarO %d\n", org->security_level_passage_var1);
		fprintf(fp, "SecPassVarT %d\n", org->security_level_passage_var2);
		fprintf(fp, "SecPassVarH %d\n", org->security_level_passage_var3);

		rank = org->ranks;
		member = org->members;

		while (rank)
		{
			fprintf(fp, "Rank '%s' %d %d %d %d %d %d %d %d %d\n",	//Check the fprintf() on intros to see how to save.
				rank->name,
				rank->default_security_general,
				rank->default_security_bank,
				rank->default_security_discharge,
				rank->default_security_admit,
				rank->default_security_passage_main,
				rank->default_security_passage_var1,
				rank->default_security_passage_var2,
				rank->default_security_passage_var3,
				rank->default_obscurity);

			rank = rank->next;
		}

		while (member)
		{
			fprintf(fp, "Member '%s' '%s' '%s' %d %d %d %d %d %d %d %d %d\n",
				member->name,
				member->rank->name,
				member->id,
				member->security_general,
				member->security_level_bank,
				member->security_level_discharge,
				member->security_level_admit,
				member->security_level_passage_main,
				member->security_level_passage_var1,
				member->security_level_passage_var2,
				member->security_level_passage_var3,
				member->obscurity);
			member = member->next;
		}

		fprintf(fp, "Doneorg\n\n");
		org = org->next;
	}

	fprintf(fp, "End\n");
	fclose(fp);

	return;
}

//load organization states from the textfile in which they are saved
void load_organizations(void)
{

	FILE *fp;
	bool fMatch;
	char *word;

	fp = fopen(ORG_DATA_FILE, "r");

	if (!fp)
	{
		bugf("%s inaccessible.  Org loading failed.", ORG_DATA_FILE);
		perror(ORG_DATA_FILE);
		return;
	}

	ORG_DATA *org;
	RANK_DATA *rank;
	MEMBER_DATA *member;

	word = fread_word(fp);

	for (; str_cmp(word, "End"); word = fread_word(fp))
	{

		if (!str_cmp(word, "Beginorg"))
		{

			org = new_org();

			org->next = org_list;
			org_list = org;

			word = fread_word(fp);

			while (str_cmp(word, "Doneorg"))
			{
				fMatch = FALSE;

				switch (UPPER(word[0]))
				{
				case '*':
					fMatch = TRUE;
					fread_to_eol(fp);
					break;

				case 'N':
					KEY("Name", org->name, fread_string(fp));
					break;

				case 'T':
					KEY("Type", org->type, fread_number(fp));
					break;

				case 'S':
					KEY("SecLeader", org->security_level_leader, fread_number(fp));
					KEY("SecBank", org->security_level_bank, fread_number(fp));
					KEY("SecRemove", org->security_level_discharge, fread_number(fp));
					KEY("SecAdmit", org->security_level_admit, fread_number(fp));
					KEY("SecPassMain", org->security_level_passage_main, fread_number(fp));
					KEY("SecPassVarO", org->security_level_passage_var1, fread_number(fp));
					KEY("SecPassVarT", org->security_level_passage_var2, fread_number(fp));
					KEY("SecPassVarH", org->security_level_passage_var3, fread_number(fp));
					break;

				case 'R':

					if (!str_cmp(word, "Rank"))
					{
						log_f("Loading %s rank.", org->name ? org->name : "(Null)");

						rank = new_rank();

						rank->name = str_dup(fread_word(fp));

						rank->default_security_general = fread_number(fp);
						rank->default_security_bank = fread_number(fp);
						rank->default_security_discharge = fread_number(fp);
						rank->default_security_admit = fread_number(fp);
						rank->default_security_passage_main = fread_number(fp);
						rank->default_security_passage_var1 = fread_number(fp);
						rank->default_security_passage_var2 = fread_number(fp);
						rank->default_security_passage_var3 = fread_number(fp);
						rank->default_obscurity = fread_number(fp);

						rank->org = org;
						rank->next = org->ranks;
						org->ranks = rank;
						fMatch = TRUE;
						break;
					}

				case 'M':

					if (!str_cmp(word, "Member"))
					{

						member = new_member();

						member->name = str_dup(fread_word(fp));

						if (!(member->rank = get_rank_by_name(org, fread_word(fp))))
						{
							bugf("No member rank found for org %s.\r\n", org->name);
							break;
						}

						member->id = fread_string(fp);
						member->security_general = fread_number(fp);
						member->security_level_bank = fread_number(fp);
						member->security_level_discharge = fread_number(fp);
						member->security_level_admit = fread_number(fp);
						member->security_level_passage_main = fread_number(fp);
						member->security_level_passage_var1 = fread_number(fp);
						member->security_level_passage_var2 = fread_number(fp);
						member->security_level_passage_var3 = fread_number(fp);
						member->obscurity = fread_number(fp);

						member->org = org;
						member->next = org->members;
						org->members = member;
						fMatch = TRUE;
						log_f("Loading %s member %s.", org->name, member->name);
						break;
					}
				}
				if (!fMatch)
				{
					bug("Load_organizations: no match.", 0);
					fread_to_eol(fp);
				}

				word = fread_word(fp);
			}

		}
	}

	if (!str_cmp(word, "End"))
	{
		log_f("Loaded... organizations");
	}

	fclose(fp);
	return;
}

//Provided with a name, returns the matching rank, returning NULL if none is found.
RANK_DATA *get_rank_by_name(ORG_DATA *org, char *name)
{
	RANK_DATA *iterator;

	for (iterator = org->ranks; iterator; iterator = iterator->next)
	{
		if (!str_cmp(name, iterator->name))
		{
			return iterator;
		}
	}

	return NULL;
}

//Takes a name, returns the org that matches.  Returns NULL if none exists.
ORG_DATA *get_org_by_name(char *name)
{
	ORG_DATA *list;

	for (list = org_list; list; list = list->next)
	{
		//		if(!str_cmp(name,list->name))
		if (is_name(name, list->name))
		{
			return list;
		}
	}

	return NULL;
}

//Takes a name, returns the character that matches in an org's member list.
//Note: this is not for comparisons.  Use get_member_by_id for that.
//Returns null if there is no such member data with that name.
MEMBER_DATA *get_member_by_name(ORG_DATA *org, char *name)
{
	MEMBER_DATA *list;

	for (list = org->members; list; list = list->next)
	{
		if (!str_cmp(name, list->name))
		{
			return list;
		}
	}

	return NULL;
}

//Gets a member by their ID number.  Returns NULL if there is no such
//member with the provided ID.
MEMBER_DATA *get_member_by_id(ORG_DATA *org, CHAR_DATA *ch)
{
	MEMBER_DATA *list;

	for (list = org->members; list; list = list->next)
	{
		if (!str_cmp(list->id, ch->uuid))
		{
			return list;
		}
	}

	return NULL;
}

//Returns TRUE if ch is a member of org.
bool is_member(CHAR_DATA *ch, ORG_DATA *org)
{
	if (!get_member_by_id(org, ch)) return FALSE;

	return TRUE;
}

//Returns TRUE if CH is a member of any org.
bool has_org(CHAR_DATA *ch)
{
	ORG_DATA *org;

	for (org = org_list; org; org = org->next)
	{
		if (get_member_by_id(org, ch))
		{
			return TRUE;
		}
	}

	return FALSE;
}

#define PERMISSION_EDIT_ORGDATA		1
#define	PERMISSION_EDIT_MEMBERDATA	2
#define	PERMISSION_EDIT_RANKDATA	3
#define PERMISSION_ADMIT		4
#define	PERMISSION_DISCHARGE		5

//check to see if a member of an organization has a permission within that org
bool check_permissions(CHAR_DATA *ch, ORG_DATA *org, int permission)
{
	if (!is_member(ch, org))
	{
		return FALSE;
	}

	MEMBER_DATA 	*member_info;
	RANK_DATA	*rank_info;
	int required, has;

	if (!(member_info = get_member_by_id(org, ch)))
	{
		return FALSE;
	}

	if (!(rank_info = member_info->rank))
	{
		return FALSE;
	}

	switch (permission)
	{
	case PERMISSION_EDIT_ORGDATA:
		required = org->security_level_leader;
		if ((has = member_info->security_general) == 0) has = rank_info->default_security_general;
		if (required < has) return TRUE;
		break;

	case PERMISSION_EDIT_MEMBERDATA:
		required = org->security_level_leader;
		if ((has = member_info->security_general) == 0) has = rank_info->default_security_general;
		if (required < has) return TRUE;

	case PERMISSION_EDIT_RANKDATA:
		required = org->security_level_leader;
		if ((has = member_info->security_general) == 0) has = rank_info->default_security_general;
		if (required < has) return TRUE;

	case PERMISSION_ADMIT:
		required = org->security_level_admit;
		if ((has = member_info->security_general) == 0) has = rank_info->default_security_general;
		if (required < has) return TRUE;

	case PERMISSION_DISCHARGE:
		required = org->security_level_discharge;
		if ((has = member_info->security_level_discharge) == 0) has = rank_info->default_security_discharge;
		if (required < has) return TRUE;

	default:
		return FALSE;
	}

	return FALSE;
}

//obvious enough: string search for organization name field
int get_org_type_by_name(char *name)
{
	int x;

	for (x = 0; org_type_table[x].name != NULL; x++)
	{
		if (!str_cmp(org_type_table[x].name, name))
		{
			return org_type_table[x].type;
		}
	}

	return -1;
}

//see if the member of an organization (MEMBER) is obscured. i.e.
//this is a check to see if the member is supposed to appear
bool check_obscurity(CHAR_DATA *ch, MEMBER_DATA *member)
{
	ORG_DATA *org;
	MEMBER_DATA *ch_member_info;
	int required;
	int has;

	if (!has_org(ch)) return FALSE;

	if (!(org = member->org)) return FALSE;

	if (!(ch_member_info = get_member_by_id(org, ch))) return FALSE;

	if ((required = member->obscurity) == 0) required = member->rank->default_obscurity;
	if ((has = ch_member_info->security_general) == 0) has = ch_member_info->rank->default_security_general;

	if (required > has) return FALSE;

	return TRUE;
}

#define SECURITY_LEADER		1
#define	SECURITY_BANK		2
#define SECURITY_ADMIT		3
#define	SECURITY_DISCHARGE	4
#define SECURITY_PASSAGE_MAIN	5
#define SECURITY_PASSAGE_VAR1	6
#define SECURITY_PASSAGE_VAR2	7
#define	SECURITY_PASSAGE_VAR3	8


//find the security rating of an individual, which may differ from their rank
int get_security(ORG_DATA *org, CHAR_DATA *ch, int permission)
{
	if (IS_IMMORTAL(ch))
	{
		return 100;
	}

	bool defaultrank = FALSE;

	if (IS_NPC(ch))
	{
		if (!get_org_by_name(ch->allegiance) ||
			get_org_by_name(ch->allegiance) != org)
			return 0;

		defaultrank = TRUE;
	}

	int has;
	MEMBER_DATA *member;

	if (!(member = get_member_by_id(org, ch)))
	{
		return 0;
	}

	switch (permission)
	{
	case SECURITY_LEADER:
		if ((has = member->security_general) == 0) has = member->rank->default_security_general;
		break;

	case SECURITY_BANK:
		if ((has = member->security_level_bank) == 0) has = member->rank->default_security_bank;
		break;

	case SECURITY_ADMIT:
		if ((has = member->security_level_admit) == 0) has = member->rank->default_security_admit;
		break;

	case SECURITY_DISCHARGE:
		if ((has = member->security_level_discharge) == 0) has = member->rank->default_security_discharge;
		break;

	case SECURITY_PASSAGE_MAIN:
		if ((has = member->security_level_passage_main) == 0) has = member->rank->default_security_passage_main;
		if (defaultrank) has = org->security_level_passage_main;
		break;

	case SECURITY_PASSAGE_VAR1:
		if ((has = member->security_level_passage_var1) == 0) has = member->rank->default_security_passage_var1;
		if (defaultrank) has = org->security_level_passage_var1;
		break;

	case SECURITY_PASSAGE_VAR2:
		if ((has = member->security_level_passage_var2) == 0) has = member->rank->default_security_passage_var2;
		if (defaultrank) has = org->security_level_passage_var2;
		break;

	case SECURITY_PASSAGE_VAR3:
		if ((has = member->security_level_passage_var3) == 0) has = member->rank->default_security_passage_var3;
		if (defaultrank) has = org->security_level_passage_var3;
		break;

	default:
		return 0;
		break;
	}

	return has;
}



/*

What else is needed?

Do_discharge
Do_removeorg
Do_orglist

Make it so orgs can have multiple words in the names?
Make get_char_by_name use is_name(), because name can store a sdesc

*/


//"orglist" command code, will list the total orglist to an admin
void do_orglist(CHAR_DATA *ch, char *argument)
{
	ORG_DATA *org;

	if (!IS_IMMORTAL(ch))
	{
		PTC(ch, "Huh?\r\n");
		return;
	}

	PTC(ch, "List of organizations:\r\n\r\n");
	for (org = org_list; org; org = org->next)
	{
		PTC(ch, "%s [%s]\r\n", org->name, org_type_table[org->type].name);
	}
	return;
}

//discharge command: player command to boot someone from an organization both members are a part of
void do_discharge(CHAR_DATA *ch, char *argument)
{
	ORG_DATA *org;
	MEMBER_DATA *member;
	char arg[MSL], arg2[MSL];

	if (!has_org(ch) && !IS_IMMORTAL(ch))
	{
		PTC(ch, "Huh?\r\n");
		return;
	}

	onearg(argument, arg);
	onearg(argument, arg2);

	if (noarg(arg))
	{
		PTC(ch, "Syntax: discharge <organization> <member>\r\n");
		return;
	}

	if (!(org = get_org_by_name(arg)))
	{
		PTC(ch, "You are not a member of that organization.\r\n");
		return;
	}

	if (!get_member_by_id(org, ch))
	{
		PTC(ch, "You are not a member of that organization.\r\n");
		return;
	}

	if (!check_permissions(ch, org, PERMISSION_DISCHARGE) && !IS_IMMORTAL(ch))
	{
		PTC(ch, "You don't have the necessary permissions to discharge players.\r\n");
		return;
	}

	if (!(member = get_member_by_name(org, arg2)))
	{
		PTC(ch, "There is no such member in your organization.\r\n");
		return;
	}

	if (!IS_IMMORTAL(ch) && (member->security_general > get_security(org, ch, SECURITY_DISCHARGE)))
	{
		PTC(ch, "You cannot discharge a member beyond your own permissions.\r\n");
		return;
	}

	PTC(ch, "You have removed %s from your organization.\r\n", member->name);
	delete_member(org, member);
	return;
}


//admit a non-member to the organization, provided that the permissions align
void do_admit(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	ORG_DATA *org;
	RANK_DATA *rank;
	MEMBER_DATA *list;
	char arg[MSL], arg2[MSL], arg3[MSL];

	if (!has_org(ch) && !IS_IMMORTAL(ch))
	{
		PTC(ch, "Huh?\r\n");
		return;
	}

	onearg(argument, arg);
	onearg(argument, arg2);
	onearg(argument, arg3);

	if (noarg(arg))
	{
		PTC(ch, "Syntax: admit <organization> <player> [rank]\r\n");
		return;
	}

	if (!(org = get_org_by_name(arg)))
	{
		PTC(ch, "You are not a member of that organization.\r\n");
		return;
	}

	if (!check_permissions(ch, org, PERMISSION_ADMIT) && !IS_IMMORTAL(ch))
	{
		PTC(ch, "You don't have the necessary permissions to admit players.\r\n");
		return;
	}

	if (IS_IMMORTAL(ch))
	{
		victim = get_char_world(NULL, arg2);
	}
	else
	{
		victim = get_char_room(ch, NULL, arg2);
	}

	if (!victim)
	{
		PTC(ch, "There is no one here by that name.\r\n");
		return;
	}

	if (noarg(arg3))
	{
		rank = get_rank_by_name(org, arg3);
	}
	else
	{
		rank = get_rank_by_name(org, "none");
	}

	if (!rank)
	{
		PTC(ch, "There is no such rank as '%s' in that organization.\r\n", arg3);
		return;
	}

	for (list = org->members; list; list = list->next)
	{
		if (!str_cmp(list->name, pers(victim, ch)) || !str_cmp(list->name, victim->short_descr))
		{
			PTC(ch, "%s is already a part of your organization.\r\n", pers(victim, ch));
			return;
		}
	}

	if (!IS_IMMORTAL(ch) && (rank->default_security_general > get_security(org, ch, SECURITY_ADMIT)))
	{
		PTC(ch, "You cannot admit a player as a rank beyond your own permissions.\r\n");
		return;
	}

	add_member(org, rank, victim, pers(victim, ch));
	PTC(ch, "%s added to organization %s as rank %s.\r\n", pers(victim, ch), org->name, rank->name);

	if (IS_IMMORTAL(ch) && ch == victim)
	{
		get_member_by_id(org, ch)->security_general = 101;
		get_member_by_id(org, ch)->security_level_bank = 101;
		get_member_by_id(org, ch)->security_level_admit = 101;
		get_member_by_id(org, ch)->security_level_discharge = 101;
		get_member_by_id(org, ch)->security_level_passage_main = 101;
		get_member_by_id(org, ch)->security_level_passage_var1 = 101;
		get_member_by_id(org, ch)->security_level_passage_var2 = 101;
		get_member_by_id(org, ch)->security_level_passage_var3 = 101;
		get_member_by_id(org, ch)->obscurity = 101;
	}

	return;
}

//admin command to create a new organization
void do_createorg(CHAR_DATA *ch, char *argument)
{
	if (!IS_IMMORTAL(ch))
	{
		PTC(ch, "Huh?\r\n");
		return;
	}

	if (noarg(argument))
	{
		PTC(ch, "Syntax: createorg <organization name>\r\n");
		return;
	}

	//Whip up a new organization.
	ORG_DATA *org;

	org = new_org();
	org->next = org_list;
	org_list = org;

	org->name = str_dup(argument);

	//let's make 1 rank and assign it to the creator
	org->ranks = new_rank();
	org->ranks->name = str_dup("Leader");
	org->ranks->default_security_general = 100;
	org->ranks->default_security_bank = 100;
	org->ranks->default_security_discharge = 100;
	org->ranks->default_security_admit = 100;
	org->ranks->default_security_passage_main = 100;
	org->ranks->default_security_passage_var1 = 100;
	org->ranks->default_security_passage_var2 = 100;
	org->ranks->default_security_passage_var3 = 100;
	org->ranks->org = org;

	//let's plop in the creator manually
	org->members = new_member();
	org->members->org = org;
	org->members->name = str_dup(ch->name);
	org->members->id = str_dup(ch->uuid);
	org->members->rank = org->ranks;
	org->members->security_general = 101;
	org->members->security_level_bank = 101;
	org->members->security_level_discharge = 101;
	org->members->security_level_admit = 101;
	org->members->security_level_passage_main = 101;
	org->members->security_level_passage_var1 = 101;
	org->members->security_level_passage_var2 = 101;
	org->members->security_level_passage_var3 = 101;

	PTC(ch, "New organization, %s, created.\r\n", org->name);
	return;
}

//admin command to completely delete an organization
void do_deleteorg(CHAR_DATA *ch, char *argument)
{
	if (!IS_IMMORTAL(ch))
	{
		PTC(ch, "Huh?\r\n");
		return;
	}

	ORG_DATA *org, *list;
	MEMBER_DATA *member, *next_member;
	RANK_DATA *rank, *next_rank;

	if (!(org = get_org_by_name(argument)))
	{
		PTC(ch, "There is no such organization.\r\n");
		return;
	}

	for (member = org->members; member; member = next_member)
	{
		next_member = member->next;
		free_member(member);
	}

	for (rank = org->ranks; rank; rank = next_rank)
	{
		next_rank = rank->next;
		free_rank(rank);
	}

	if (org_list == org)
	{
		org_list = org->next;
	}

	else
	{
		for (list = org_list; list; list = list->next)
		{
			if (list->next == org)
			{
				list->next = org->next;
			}
		}
	}

	free_org(org);
	PTC(ch, "Organization deleted.\r\n");
	return;
}

//player/admin command to edit the status of a rank.
//creates a bit of a wall of text for the player but it's necessary

void do_rankedit(CHAR_DATA *ch, char *argument)
{
	ORG_DATA *org;
	RANK_DATA *rank;
	char arg[MSL], arg2[MSL], arg3[MSL], arg4[MSL];

	if (!has_org(ch) && !IS_IMMORTAL(ch))
	{
		PTC(ch, "Huh?\r\n");
		return;
	}

	onearg(argument, arg);

	if (noarg(arg))
	{
		PTC(ch, "Syntax: rankedit <organization>\r\n");
		PTC(ch, "        rankedit <organization> [add/remove] <name>\r\n");
		PTC(ch, "        rankedit <organization> <rank>\r\n");
		PTC(ch, "        rankedit <organization> <rank> <field> <value>\r\n");
		return;
	}

	org = get_org_by_name(arg);

	if (!org || (!is_member(ch, org) && !IS_IMMORTAL(ch)))
	{
		PTC(ch, "You are not a member of that organization.\r\n");
		return;
	}

	if (!check_permissions(ch, org, PERMISSION_EDIT_RANKDATA) && !IS_IMMORTAL(ch))
	{
		PTC(ch, "You don't have the necessary permissions to edit that organization.\r\n");
		return;
	}

	order_ranks(org->ranks);

	onearg(argument, arg2);

	if (noarg(arg2))
	{
		int has;

		if ((has = get_member_by_id(org, ch)->security_general) == 0)
			has = get_member_by_id(org, ch)->rank->default_security_general;

		PTC(ch, "Current ranks of %s:\r\n\r\n", org->name);
		for (rank = org->ranks; rank; rank = rank->next)
		{

			if (rank->default_obscurity < has)
			{
				;
				PTC(ch, "%-25s [default security %d].\r\n", rank->name, rank->default_security_general);
			}
		}
		return;
	}

	argument = first_arg(argument, arg3, FALSE);
	onearg(argument, arg4);

	if (!str_cmp(arg2, "add"))
	{
		if (noarg(arg3))
		{
			PTC(ch, "Syntax: rankedit <organization> add <name>\r\n");
			return;
		}

		rank = new_rank();

		rank->next = org->ranks;
		org->ranks = rank;
		rank->org = org;

		rank->name = str_dup(arg3);
		PTC(ch, "You have created the '%s' rank.\r\n", rank->name);
		return;
	}

	if (!str_cmp(arg2, "remove"))
	{
		if (noarg(arg3))
		{
			PTC(ch, "Syntax: rankedit <organization> remove <name>\r\n");
			return;
		}

		if ((rank = get_rank_by_name(org, arg3)) == NULL)
		{
			PTC(ch, "Your organization has no such rank.\r\n");
			return;
		}

		PTC(ch, "Deleting rank: %s\r\n", rank->name);
		delete_rank(rank);
		return;
	}

	if (!(rank = get_rank_by_name(org, arg2)))
	{
		PTC(ch, "Your organization has no such rank.\r\n");
		return;
	}

	if (noarg(arg3))
	{
		PTC(ch, "Listing details for rank: %s.\r\n\r\n", rank->name);
		PTC(ch, "%s%s\r\n", "Name: ", rank->name);
		PTC(ch, "%-35s%d\r\n", "Default General Security:", rank->default_security_general);
		PTC(ch, "%-35s%d\r\n", "Default Banking Security:", rank->default_security_bank);
		PTC(ch, "%-35s%d\r\n", "Default Admitting Security:", rank->default_security_admit);
		PTC(ch, "%-35s%d\r\n", "Default Discharging Security:", rank->default_security_discharge);
		PTC(ch, "%-35s%d\r\n", "Default Main Passage Security:", rank->default_security_passage_main);
		PTC(ch, "%-35s%d\r\n", "Default Second Passage Security:", rank->default_security_passage_var1);
		PTC(ch, "%-35s%d\r\n", "Default Third Passage Security:", rank->default_security_passage_var2);
		PTC(ch, "%-35s%d\r\n", "Default Fourth Passage Security:", rank->default_security_passage_var3);
		PTC(ch, "%-35s%d\r\n", "Default Obscurity:", rank->default_obscurity);
		return;
	}

	if (noarg(arg4))
	{
		PTC(ch, "Syntax: rankedit <organization>\r\n");
		PTC(ch, "        rankedit <organization> [add/remove] <name>\r\n");
		PTC(ch, "        rankedit <organization> <rank>\r\n");
		PTC(ch, "        rankedit <organization> <rank> <field> <value>\r\n");
		return;
	}

	if (!str_cmp(arg4, "name"))
	{

		free_string(rank->name);
		rank->name = str_dup(arg4);

		PTC(ch, "Rank name changed to: %s.\r\n", rank->name);
		return;
	}

	if (!is_number(arg4))
	{
		PTC(ch, "Syntax: rankedit <organization>\r\n");
		PTC(ch, "        rankedit <organization> [add/remove] <name>\r\n");
		PTC(ch, "        rankedit <organization> <rank>\r\n");
		PTC(ch, "        rankedit <organization> <rank> <field> <value>\r\n");
		return;
	}


	int ivalue = atoi(arg4);

	if (ivalue < 0 || ivalue > 100)
	{
		PTC(ch, "Valid range is from 0 to 100.\r\n");
		return;
	}

	if (!str_cmp(arg3, "General"))
	{
		PTC(ch, "Changing rank %s default general security from '%d' to ", rank->name, rank->default_security_general);
		rank->default_security_general = ivalue;
		PTC(ch, "'%d'.\r\n", rank->default_security_general);
		return;
	}

	if (!str_cmp(arg3, "Bank"))
	{
		PTC(ch, "Changing rank %s default banking security from '%d' to ", rank->name, rank->default_security_bank);
		rank->default_security_bank = ivalue;
		PTC(ch, "'%d'.\r\n", rank->default_security_bank);
		return;
	}

	if (!str_cmp(arg3, "Admit"))
	{
		PTC(ch, "Changing rank %s default admitting security from '%d' to ", rank->name, rank->default_security_admit);
		rank->default_security_admit = ivalue;
		PTC(ch, "'%d'.\r\n", rank->default_security_admit);
		return;
	}

	if (!str_cmp(arg3, "Discharge"))
	{
		PTC(ch, "Changing rank %s default discharging security from '%d' to ", rank->name, rank->default_security_discharge);
		rank->default_security_discharge = ivalue;
		PTC(ch, "'%d'.\r\n", rank->default_security_discharge);
		return;
	}

	if (!str_cmp(arg3, "Main"))
	{
		PTC(ch, "Changing rank %s default main passage security from '%d' to ", rank->name, rank->default_security_passage_main);
		rank->default_security_passage_main = ivalue;
		PTC(ch, "'%d'.\r\n", rank->default_security_passage_main);
		return;
	}

	if (!str_cmp(arg3, "Second"))
	{
		PTC(ch, "Changing rank %s default second passage security from '%d' to ", rank->name, rank->default_security_passage_var1);
		rank->default_security_passage_var1 = ivalue;
		PTC(ch, "'%d'.\r\n", rank->default_security_passage_var1);
		return;
	}

	if (!str_cmp(arg3, "Third"))
	{
		PTC(ch, "Changing rank %s default third passage security from '%d' to ", rank->name, rank->default_security_passage_var2);
		rank->default_security_passage_var2 = ivalue;
		PTC(ch, "'%d'.\r\n", rank->default_security_passage_var2);
		return;
	}

	if (!str_cmp(arg3, "Fourth"))
	{
		PTC(ch, "Changing rank %s default fourth passage security from '%d' to ", rank->name, rank->default_security_passage_var3);
		rank->default_security_passage_var3 = ivalue;
		PTC(ch, "'%d'.\r\n", rank->default_security_passage_var3);
		return;
	}

	if (!str_cmp(arg3, "Obscurity"))
	{
		PTC(ch, "Changing rank %s default obscurity from '%d' to ", rank->name, rank->default_obscurity);
		rank->default_obscurity = ivalue;
		PTC(ch, "'%d'.\r\n", rank->default_obscurity);
		return;
	}


	PTC(ch, "Syntax: rankedit <organization>\r\n");
	PTC(ch, "        rankedit <organization> [add/remove] <name>\r\n");
	PTC(ch, "        rankedit <organization> <rank>\r\n");
	PTC(ch, "        rankedit <organization> <rank> <field> <value>\r\n");
	return;
}


//player/admin command, edits the default settings for an organization
void do_orgedit(CHAR_DATA *ch, char *argument)
{
	ORG_DATA *org;
	char arg[MSL], field[MSL], value[MSL];

	if (!has_org(ch))
	{
		PTC(ch, "Huh?\r\n");
		return;
	}

	onearg(argument, arg);

	if (noarg(arg))
	{
		PTC(ch, "Syntax: orgedit <organization>\r\n");
		PTC(ch, "        orgedit <organization> <field> <value>\r\n");
		return;
	}

	org = get_org_by_name(arg);

	if (!org || !is_member(ch, org))
	{
		PTC(ch, "You are not a member of that organization.\r\n");
		return;
	}

	if (!check_permissions(ch, org, PERMISSION_EDIT_ORGDATA))
	{
		PTC(ch, "You don't have the necessary permissions to edit that organization.\r\n");
		return;
	}

	onearg(argument, field);

	if (noarg(field))
	{
		PTC(ch, "Listing details for organization: %s.\r\n\r\n", org->name);
		PTC(ch, "%s: %s\r\n", "Name", org->name);
		PTC(ch, "%s: %s\r\n\r\n", "Type", org_type_table[org->type].name);
		PTC(ch, "%-35s%d\r\n", "Minimum Leader Security:", org->security_level_leader);
		PTC(ch, "%-35s%d\r\n", "Minimum Banking Security:", org->security_level_bank);
		PTC(ch, "%-35s%d\r\n", "Minimum Admitting Security:", org->security_level_admit);
		PTC(ch, "%-35s%d\r\n", "Minimum Discharging Security:", org->security_level_discharge);
		PTC(ch, "%-35s%d\r\n", "Minimum Main Passage Security:", org->security_level_passage_main);
		PTC(ch, "%-35s%d\r\n", "Minimum Second Passage Security:", org->security_level_passage_var1);
		PTC(ch, "%-35s%d\r\n", "Minimum Third Passage Security:", org->security_level_passage_var2);
		PTC(ch, "%-35s%d\r\n", "Minimum Fourth Passage Security:", org->security_level_passage_var3);
		return;
	}

	onearg(argument, value);

	if (noarg(value))
	{
		PTC(ch, "Syntax: orgedit <organization>\r\n");
		PTC(ch, "        orgedit <organization> <field> <value>\r\n");
		return;
	}

	if (!str_cmp(field, "name"))
	{
		if (!IS_IMMORTAL(ch))
		{
			PTC(ch, "Only staff members may change organization names.\r\n");
			return;
		}

		free_string(org->name);
		org->name = str_dup(value);

		PTC(ch, "Organization name changed to: %s.\r\n", org->name);
		return;
	}

	if (!str_cmp(field, "type"))
	{
		if (!IS_IMMORTAL(ch))
		{
			PTC(ch, "Only staff members may change organization types.\r\n");
			return;
		}

		if (get_org_type_by_name(value) == -1)
		{
			PTC(ch, "No such organization type.\r\n");
			return;
		}

		PTC(ch, "Org type changed from '%s' to ", org_type_table[org->type].name);
		org->type = get_org_type_by_name(value);
		PTC(ch, "'%s'.\r\n", org_type_table[org->type].name);
		return;
	}

	if (!is_number(value))
	{
		PTC(ch, "Syntax: orgedit <organization>\r\n");
		PTC(ch, "        orgedit <organization> <field> <value>\r\n");
		return;
	}

	int ivalue = atoi(value);

	if (ivalue < 0 || ivalue > 100)
	{
		PTC(ch, "Valid range is from 0 to 100.\r\n");
		return;
	}

	if (!str_cmp(field, "Leader"))
	{
		PTC(ch, "Changing the minimum leader security from '%d' to ", org->security_level_leader);
		org->security_level_leader = ivalue;
		PTC(ch, "'%d'.\r\n", org->security_level_leader);
		return;
	}

	if (!str_cmp(field, "Bank"))
	{
		PTC(ch, "Changing the minimum banking security from '%d' to ", org->security_level_bank);
		org->security_level_bank = ivalue;
		PTC(ch, "'%d'.\r\n", org->security_level_bank);
		return;
	}

	if (!str_cmp(field, "Admit"))
	{
		PTC(ch, "Changing the minimum admitting security from '%d' to ", org->security_level_admit);
		org->security_level_admit = ivalue;
		PTC(ch, "'%d'.\r\n", org->security_level_admit);
		return;
	}

	if (!str_cmp(field, "Discharge"))
	{
		PTC(ch, "Changing the minimum discharging security from '%d' to ", org->security_level_discharge);
		org->security_level_discharge = ivalue;
		PTC(ch, "'%d'.\r\n", org->security_level_discharge);
		return;
	}

	if (!str_cmp(field, "Main"))
	{
		PTC(ch, "Changing the minimum main passage security from '%d' to ", org->security_level_passage_main);
		org->security_level_passage_main = ivalue;
		PTC(ch, "'%d'.\r\n", org->security_level_passage_main);
		return;
	}

	if (!str_cmp(field, "Second"))
	{
		PTC(ch, "Changing the minimum second passage security from '%d' to ", org->security_level_passage_var1);
		org->security_level_passage_var1 = ivalue;
		PTC(ch, "'%d'.\r\n", org->security_level_passage_var1);
		return;
	}

	if (!str_cmp(field, "Third"))
	{
		PTC(ch, "Changing the minimum third passage security from '%d' to ", org->security_level_passage_var2);
		org->security_level_passage_var2 = ivalue;
		PTC(ch, "'%d'.\r\n", org->security_level_passage_var2);
		return;
	}

	if (!str_cmp(field, "Fourth"))
	{
		PTC(ch, "Changing the minimum fourth passage security from '%d' to ", org->security_level_passage_var3);
		org->security_level_passage_var3 = ivalue;
		PTC(ch, "'%d'.\r\n", org->security_level_passage_var3);
		return;
	}

	PTC(ch, "Syntax: orgedit <organization>\r\n");
	PTC(ch, "        orgedit <organization> <field> <value>\r\n");
	return;
}

//edit the default status of a PARTICULAR organizational member
//this stores values that overwrite the RANK status
void do_memberedit(CHAR_DATA *ch, char *argument)
{
	ORG_DATA *org;
	MEMBER_DATA *member;
	char arg[MSL], member_name[MSL], field[MSL], value[MSL];

	if (!has_org(ch))
	{
		PTC(ch, "Huh?\r\n");
		return;
	}

	onearg(argument, arg);

	if (noarg(arg))
	{
		PTC(ch, "Syntax: memberedit <organization>\r\n");
		PTC(ch, "        memberedit <organization> <member>\r\n");
		PTC(ch, "        memberedit <organization> <member> <field> <value>\r\n");
		return;
	}

	org = get_org_by_name(arg);

	if (!org || !is_member(ch, org))
	{
		PTC(ch, "You are not a member of that organization.\r\n");
		return;
	}

	if (!check_permissions(ch, org, PERMISSION_EDIT_MEMBERDATA))
	{
		PTC(ch, "You don't have the necessary permissions to edit that organization.\r\n");
		return;
	}

	onearg(argument, member_name);

	if (noarg(member_name))
	{
		MEMBER_DATA *list;
		PTC(ch, "Current members of %s:\r\n", org->name);
		for (list = org->members; list; list = list->next)
		{
			if (check_obscurity(ch, list))
			{
				PTC(ch, "%s, rank %s.\r\n", list->name, list->rank->name);
			}
		}
		return;
	}


	member = get_member_by_name(org, member_name);

	if (!member || !check_obscurity(ch, member))
	{
		PTC(ch, "There is no one by that name in your organization.\r\n");
		return;
	}

	onearg(argument, field);

	if (noarg(field))
	{
		PTC(ch, "Listing details for member: %s.\r\n\r\n", member->name);
		PTC(ch, "%s %s\r\n", "Name:", member->name);
		PTC(ch, "%s %s\r\n\r\n", "Rank:", member->rank);
		PTC(ch, "%-35s%d\r\n", "Banking Security:", member->security_level_bank);
		PTC(ch, "%-35s%d\r\n", "Admitting Security:", member->security_level_admit);
		PTC(ch, "%-35s%d\r\n", "Discharging Security:", member->security_level_discharge);
		PTC(ch, "%-35s%d\r\n", "Main Passage Security:", member->security_level_passage_main);
		PTC(ch, "%-35s%d\r\n", "Second Passage Security:", member->security_level_passage_var1);
		PTC(ch, "%-35s%d\r\n", "Third Passage Security:", member->security_level_passage_var2);
		PTC(ch, "%-35s%d\r\n", "Fourth Passage Security:", member->security_level_passage_var3);
		return;
	}

	onearg(argument, value);

	if (noarg(value))
	{
		PTC(ch, "Syntax: memberedit <organization>\r\n");
		PTC(ch, "        memberedit <organization> <member>\r\n");
		PTC(ch, "        memberedit <organization> <member> <field> <value>\r\n");
		return;
	}

	if (!str_cmp(field, "name"))
	{
		if (!IS_IMMORTAL(ch))
		{
			PTC(ch, "That field cannot be edited.\r\n");
			return;
		}

		free_string(member->name);
		member->name = str_dup(value);

		PTC(ch, "Member name changed to: %s.\r\n", member->name);
		return;
	}

	if (!str_cmp(field, "Rank"))
	{
		RANK_DATA *rank;

		if (!(rank = get_rank_by_name(org, value)))
		{
			PTC(ch, "Your organization has '%s' rank.\r\n", value);
			return;
		}

		PTC(ch, "%s's rank changed from '%s' to ", member->name, member->rank->name);
		member->rank = rank;
		PTC(ch, "'%s'.\r\n", member->rank->name);
		return;
	}

	if (!is_number(value))
	{
		PTC(ch, "Syntax: memberedit <organization>\r\n");
		PTC(ch, "        memberedit <organization> <member>\r\n");
		PTC(ch, "        memberedit <organization> <member> <field> <value>\r\n");
		return;
	}

	int ivalue = atoi(value);

	if (ivalue < 0 || ivalue > 100)
	{
		PTC(ch, "Valid range is from 0 to 100.\r\n");
		return;
	}

	if (!str_cmp(field, "General"))
	{
		PTC(ch, "Changing %'s general security from '%d' to ", member->name, member->name, member->security_general);
		member->security_general = ivalue;
		PTC(ch, "'%d'.\r\n", member->security_general);
		return;
	}

	if (!str_cmp(field, "Bank"))
	{
		PTC(ch, "Changing %'s banking security from '%d' to ", member->name, member->name, member->security_level_bank);
		member->security_level_bank = ivalue;
		PTC(ch, "'%d'.\r\n", member->security_level_bank);
		return;
	}

	if (!str_cmp(field, "Admit"))
	{
		PTC(ch, "Changing %'s admitting security from '%d' to ", member->name, member->security_level_admit);
		member->security_level_admit = ivalue;
		PTC(ch, "'%d'.\r\n", member->security_level_admit);
		return;
	}

	if (!str_cmp(field, "Discharge"))
	{
		PTC(ch, "Changing %'s discharging security from '%d' to ", member->name, member->security_level_discharge);
		member->security_level_discharge = ivalue;
		PTC(ch, "'%d'.\r\n", member->security_level_discharge);
		return;
	}

	if (!str_cmp(field, "Main"))
	{
		PTC(ch, "Changing %'s main passage security from '%d' to ", member->name, member->security_level_passage_main);
		member->security_level_passage_main = ivalue;
		PTC(ch, "'%d'.\r\n", member->security_level_passage_main);
		return;
	}

	if (!str_cmp(field, "Second"))
	{
		PTC(ch, "Changing %'s second passage security from '%d' to ", member->name, member->security_level_passage_var1);
		member->security_level_passage_var1 = ivalue;
		PTC(ch, "'%d'.\r\n", member->security_level_passage_var1);
		return;
	}

	if (!str_cmp(field, "Third"))
	{
		PTC(ch, "Changing %'s third passage security from '%d' to ", member->name, member->security_level_passage_var2);
		member->security_level_passage_var2 = ivalue;
		PTC(ch, "'%d'.\r\n", member->security_level_passage_var2);
		return;
	}

	if (!str_cmp(field, "Fourth"))
	{
		PTC(ch, "Changing %'s fourth passage security from '%d' to ", member->name, member->security_level_passage_var3);
		member->security_level_passage_var3 = ivalue;
		PTC(ch, "'%d'.\r\n", member->security_level_passage_var3);
		return;
	}

	if (!str_cmp(field, "Obscurity"))
	{
		PTC(ch, "Changing %'s obscurity from '%d' to ", member->name, member->obscurity);
		member->obscurity = ivalue;
		PTC(ch, "'%d'.\r\n", member->obscurity);
		return;
	}

	PTC(ch, "Syntax: memberedit <organization>\r\n");
	PTC(ch, "        memberedit <organization> <member>\r\n");
	PTC(ch, "        memberedit <organization> <member> <field> <value>\r\n");
	return;
}

//accepts a string, finds the associated integer DECLARE for the security type
int get_org_security_by_name(const char *name)
{

	if (!str_cmp(name, "leader")) return SECURITY_LEADER;
	if (!str_cmp(name, "main")) return SECURITY_PASSAGE_MAIN;
	if (!str_cmp(name, "var1")) return SECURITY_PASSAGE_VAR1;
	if (!str_cmp(name, "var2")) return SECURITY_PASSAGE_VAR2;
	if (!str_cmp(name, "var3")) return SECURITY_PASSAGE_VAR3;

	return 0;
}

//checks if a player is allowed to enter a room according to organization settings
bool can_pass_org_security(CHAR_DATA *ch, ORG_DATA *org, int type)
{
	if (type == SECURITY_LEADER)
	{
		if (!get_member_by_id(org, ch) ||
			get_member_by_id(org, ch)->security_general >= org->security_level_leader)
			return TRUE;
	}
	if (type == SECURITY_PASSAGE_MAIN)
	{
		if (!get_member_by_id(org, ch) ||
			get_member_by_id(org, ch)->security_level_passage_main >= org->security_level_passage_main)
			return TRUE;
	}

	if (type == SECURITY_PASSAGE_VAR1)
	{
		if (!get_member_by_id(org, ch) ||
			get_member_by_id(org, ch)->security_level_passage_var1 >= org->security_level_passage_var1)
			return TRUE;
	}

	if (type == SECURITY_PASSAGE_VAR2)
	{
		if (!get_member_by_id(org, ch) ||
			get_member_by_id(org, ch)->security_level_passage_var2 >= org->security_level_passage_var2)
			return TRUE;
	}

	if (type == SECURITY_PASSAGE_VAR3)
	{
		if (!get_member_by_id(org, ch) ||
			get_member_by_id(org, ch)->security_level_passage_var3 >= org->security_level_passage_var3)
			return TRUE;
	}

	return FALSE;
}
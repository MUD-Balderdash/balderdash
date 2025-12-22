/**************************************************************************
***************************************************************************/
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"


RECIPE_DATA *recipe_list;

void load_recipes(void)
{
    FILE *fprecipe;

    if ((fprecipe = file_open(RECIPE_FILE, "r")) == NULL)
    {
	_perror(RECIPE_FILE);
	exit(1);
    }

    for (; ;)
    {
	RECIPE_DATA *pRecipe;
	char *word;
	int i;

	if (fread_letter(fprecipe) != '#')
	{
	    bugf("Load_recipe: # not found.");
	    exit(1);
	}

	word = fread_word(fprecipe);
	if (!str_cmp(word, "END"))
	    break;
	
	pRecipe         = new_recipe();
	pRecipe->name   = str_dup(fread_string(fprecipe));
	pRecipe->level  = fread_number(fprecipe);
	pRecipe->rlevel = fread_number(fprecipe);
	pRecipe->reserve= fread_number(fprecipe);
	pRecipe->comp   = fread_number(fprecipe);
	pRecipe->type   = fread_number(fprecipe);
	for (i = 0; i!= MAX_ING;i++)
	    pRecipe->value[i] = fread_number(fprecipe);

	if (recipe_list == NULL)
	    recipe_list = pRecipe;
	else
	{
	    pRecipe->next = recipe_list;
	    recipe_list = pRecipe;
	}
    }
    file_close(fprecipe);
    return;
}

RECIPE_DATA *add_recipe(char *name)
{
    RECIPE_DATA *recipe;

    recipe = new_recipe();
    recipe->name = str_dup(name);

    if (recipe_list == NULL)
	recipe_list = recipe;
    else
    {
	recipe->next = recipe_list;
	recipe_list = recipe;
    }

    save_recipes();
    return recipe;
}

char *get_name_ing(int num)
{
    OBJ_INDEX_DATA *Obj;

    if ((Obj = get_obj_index(num)) != NULL)
	return Obj->short_descr;

    bugf("Задан несуществующий ингредиент.");
    return "None";
}

void show_recipe(CHAR_DATA *ch, RECIPE_DATA *recipe)
{
    char buf[MSL];
    int i;

    sprintf(buf, "Name         : %s\n\r",recipe->name);
    send_to_char(buf,ch);
    sprintf(buf, "Level min    : %d\n\r",recipe->level);
    send_to_char(buf,ch);
    sprintf(buf, "Recipe level : %d\n\r",recipe->rlevel);
    send_to_char(buf,ch);
    sprintf(buf, "Comp         : %d\n\r",recipe->comp);
    send_to_char(buf,ch);
    sprintf(buf, "Type         : %s\n\r",
	(recipe->type == 0 ? "Не задан" : 
	 recipe->type == 1 ? "Зелье" :
	 recipe->type == 2 ? "Пилюля" :
	 recipe->type == 3 ? "Танк" :
	 recipe->type == 4 ? "Дамагер" :
	 recipe->type == 5 ? "Маг" : "Жрец"));
    send_to_char(buf,ch);
    send_to_char("Inrgedients\n\r",ch);
    for (i = 0; i!= MAX_ING; i++)
    {
	sprintf(buf, "[v%d] : %s\n\r",i, 
	    (recipe->value[i] > 0 ? capitalize(get_name_ing(recipe->value[i])) : "Не задан"));
	send_to_char(buf,ch);
    }
    return;
}

void help_recipe(CHAR_DATA *ch)
{
    send_to_char("{D   recipe help/помощь{x\n\r"
		"Справка по работе с командой recipe.\n\r",ch);
    send_to_char("{D   recipe create/создать <название рецепта>{x\n\r" 
		"Создается новый рецепт, название выбирается из списка заклинаний.\n\r",ch);
    send_to_char("{D   recipe save/сохранить{x\n\r" 
		"Сохраняет рецепты.\n\r",ch);
    send_to_char("{D   recipe edit/изменить <название или номер рецепта> <параметр> <значение>{x\n\r"
		"Изменяемые параметры:\n\r"
		"name   - меняется название рецепта гомункула.\n\r"
		"level  - уровень, после которого чар может создать рецепт.\n\r"
		"rlevel - уровень сложности рецепта.\n\r"
		"comp  - процент совместимости этого рецепта с другими.\n\r"
		"type  - тип рецепта (potion, pill, tank, dam, mage, cleric).\n\r"
		"    potion - рецепт зелья, pill - рецепт пилюли,\n\r"
		"    tank   - танк    (много здоровья, малые повреждения, большая броня)\n\r"
		"    dam    - дамагер (мало здоровья, большие повреждения, малая броня)\n\r"
		"    mage   - маг     (мало здоровья, магическая атака, магическая броня)\n\r"
		"    cleric - жрец    (среднее здоровье, среднее повреждение , средняя броня)\n\r"
		"v0, v1,... - внумы ингредиентов.\n\r"
		"Запись ингредиентов можно делать списком (recipe edit 10 v 1900 1901 1902)\n\r",ch); 
    send_to_char("{D   recipe delete/удалить <название рецепта>{x\n\r"
		"Удаляем рецепт по его названию.\n\r",ch);
    send_to_char("{D   recipe show/показать <название рецепта>{x\n\r"
		"Показывает текущие настройки рецепта по его названию.\n\r",ch);
    return;
}

void edit_recipe(CHAR_DATA *ch, char *argument)
{
    RECIPE_DATA *recipe;
    char arg1[MIL];
    char arg2[MIL];
    bool cast = FALSE;
    int sn;
       	
    argument = one_argument(argument, arg1);
    if (is_number(arg1))
	recipe = get_recipe(atoi(arg1));
    else
	recipe = find_recipe(arg1);

    if (recipe != NULL)
    {
	cast = ((sn = skill_lookup(recipe->name)) != -1);

	argument = one_argument(argument, arg2);
	switch (UPPER(arg2[0]))
	{	
	int a;

	case 'N':
	    if (!str_prefix(arg2,"name") && !cast){
			free_string(recipe->name);
			recipe->name = str_dup(argument);
	    } else {
			send_to_char("{YЗаклинаниям нельзя менять название.{x\n\r\n\r",ch);
		}
	    break;
	case 'V':
	    a = arg2[1] - 48;
	    if (a == -48)
		a = 0;
	    if (a >= 0 && a < MAX_ING)
	    while (argument[0] != '\0')
	    {
		argument = one_argument(argument, arg2);		
		if (a >= 0 && a < MAX_ING
		&& is_number(arg2)
		&& (atoi(arg2) == 0 || get_obj_index(atoi(arg2)) != NULL))
		    recipe->value[a++] = atoi(arg2);
	    }
	    else
		send_to_char("{YНеправильно введен параметр ингредиента.{x\n\r\n\r",ch);
	    break;
	case 'L':
	    if (!str_prefix(arg2,"level") && is_number(argument))
	    {
		a = atoi(argument);
		if (a > 0 && a <= 60)
		    recipe->level = a;
		else
		    send_to_char("{YЗначение должно быть в пределах 1 - 60.{x\n\r\n\r",ch);
	    }
	    else
		send_to_char("{YНеправильно введен параметр уровня.{x\n\r\n\r",ch);
	    break;
	case 'R':	    
	    if (!str_prefix(arg2,"rlevel") && is_number(argument))
	    {
		a = atoi(argument);
		if (a >= 1 && a <= 6)
		    recipe->rlevel = a;
		else
		    send_to_char("{YЗначение должно быть в пределах 1 - 6.{x\n\r\n\r",ch);
	    }
	    else
		send_to_char("{YНеправильно введен параметр уровня.{x\n\r\n\r",ch);
	    break;
	case 'T':
	    if (!str_prefix(arg2,"type"))
	    {	
		if (cast && !str_prefix(argument,"potion"))
		    recipe->type = RECIPE_POTION;
		else if (cast && !str_prefix(argument,"pill"))
		    recipe->type = RECIPE_PILL;
		else if (!str_prefix(argument,"tank"))
		    recipe->type = RECIPE_TANK;
		else if (!str_prefix(argument,"dam"))
		    recipe->type = RECIPE_DAM;
		else if (!str_prefix(argument,"mage"))
		    recipe->type = RECIPE_MAGE;
		else if (!str_prefix(argument,"cleric"))
		    recipe->type = RECIPE_CLERIC;
		else
	    	    send_to_char("{YНеправильно введен тип рецепта, или название не подходит под тип.{x\n\r\n\r",ch);
	    }
	    break;
	case 'C':
	    if (!str_prefix(arg2,"comp") && is_number(argument))
	    {
		a = atoi(argument);
		if (a >= 0 && a <= 100)
		    recipe->comp = a;
		else
		    send_to_char("{YЗначение должно быть в пределах 0 - 100.{x\n\r\n\r",ch);
	    }
	    else
		send_to_char("{YНеправильно введен параметр совместимости.{x\n\r\n\r",ch);
	    break;
	}
	show_recipe(ch,recipe);	
    }
    else
	send_to_char("Не найден рецепт.\n\r",ch);
}

void list_recipe(CHAR_DATA *ch)
{
    RECIPE_DATA *recipe;
    int i, t = 1;
    char buf[MSL];

    send_to_char(" N.   Название                  Тип   Уровень Совм.  Ингредиенты\n\r",ch);
    send_to_char("====|=========================|======|=======|=====|============================================\n\r", ch);
    for (recipe  = recipe_list; recipe != NULL; recipe  = recipe->next)
    {
	sprintf(buf, "%-3d | %-23s |%-6s| %-2d|%-2d | %-3d |",t++,recipe->name,
	(recipe->type == 0 ? "Нет" : 
	 recipe->type == 1 ? "Зелье" :
	 recipe->type == 2 ? "Пилюля" :
	 recipe->type == 3 ? "Танк" :
	 recipe->type == 4 ? "Дамаг" :
	 recipe->type == 5 ? "Маг" : "Жрец"),
	recipe->level,recipe->rlevel,recipe->comp);
	send_to_char(buf,ch);
	for (i = 0; i != MAX_ING; i++)
	    if (recipe->value[i] != 0)
	    {
		sprintf(buf, "%d ",recipe->value[i]);
		send_to_char(buf,ch);
	    }
	send_to_char("\n\r",ch);
    }
    send_to_char("=================================================================================================\n\r", ch);
}                                                                                                            

RECIPE_DATA *find_recipe(char *name)
{
    RECIPE_DATA *recipe;
    int sn = skill_lookup(name);

    for (recipe  = recipe_list;
	 recipe != NULL;
	 recipe  = recipe->next)
    {
	if ((sn != -1 && skill_lookup(recipe->name) == sn)
	|| str_str(recipe->name, name))
	    break;
    }
    return recipe;
}

RECIPE_DATA *get_recipe(int num)
{
    RECIPE_DATA *recipe;
    int i = 1;

    for (recipe  = recipe_list;
	 recipe != NULL;
	 recipe  = recipe->next)
    {
	if (i++ == num)
	    break;
    }
    return recipe;
}

void delete_recipe(RECIPE_DATA *recipe)
{
    if (recipe == NULL)
	return;

    if (recipe == recipe_list)
    {
	recipe_list = recipe->next;
    }
    else
    {
	RECIPE_DATA *prev;

	for (prev = recipe_list; prev != NULL; prev = prev->next)
	{
	    if (prev->next == recipe)
	    {
		prev->next = recipe->next;
		break;
	    }
	}

	if (prev == NULL)
	    return;
    }
    free_recipe(recipe);
    save_recipes();
    return;
}

void save_recipes(void)
{
    FILE *fprecipe;
    RECIPE_DATA *recipe;

    if ((fprecipe = file_open(RECIPE_FILE, "w")) == NULL)
    {
	bugf("Save_recipes: Cannot open RECIPE_FILE file.");
	_perror(RECIPE_FILE);
    }
    else
    {
	for (recipe  = recipe_list;
	     recipe != NULL;
	     recipe  = recipe->next)
	{
	    int i;

	    fprintf(fprecipe, "#0\n");
	    fprintf(fprecipe, "%s~\n", recipe->name);
	    fprintf(fprecipe, "%d\n", recipe->level);
	    fprintf(fprecipe, "%d\n", recipe->rlevel);
	    fprintf(fprecipe, "%d\n", recipe->reserve);
	    fprintf(fprecipe, "%d\n", recipe->comp);
	    fprintf(fprecipe, "%d\n", recipe->type);
	    for (i = 0; i != MAX_ING; i++)
		fprintf(fprecipe, "%d ", recipe->value[i]);
	    fprintf(fprecipe, "\n");
	}
	fprintf(fprecipe, "#END\n\r");
    }
    file_close(fprecipe);

    return;
}

/* charset=cp1251 */

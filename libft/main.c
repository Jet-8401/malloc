#include "libft.h"

#define BUF_SIZE 20

char	changeC(unsigned int index, char c)
{
	return (c + index);
}

void	changeString(unsigned int index, char *string)
{
	(void) index;
	(void) string;
	return ;
}

int	main(void)
{
	char	ptr[BUF_SIZE];
	char	dst[BUF_SIZE * 2];

	ft_atoi("   -58461");
	ft_itoa(0x7FFFFFFF);	
	ft_bzero(ptr, sizeof(char) * BUF_SIZE);
	free(ft_calloc(514, sizeof(char)));
	int	i = 0;
	char **split = ft_split("Moulinette de merde", ' ');
	while (split[i])
		free(split[i++]);
	free(split);

	ft_isalnum('5');
	ft_isalnum('h');
	ft_isalpha('5');
	ft_isalpha('d');
	ft_isascii('5');
	ft_isdigit('5');
	ft_isprint('5');

	free(ft_memchr("AAAAAAAAAAAAAAAAAH", ' ', 10));
	ft_memcpy(dst, "Toujours pas", 5);
	ft_memmove(dst, ptr, 20);
	ft_memcmp(dst, ptr, 10);
	ft_memset(ptr, 0, BUF_SIZE);

	ft_strchr(dst, 0);
	ft_strdup("Hello");
	ft_strlcat(dst, "Prout", 5);
	ft_strlcpy(dst, "Prout", 2);
	ft_strlen("Oui");
	ft_strnstr("AAAAAAAAAAAAAAAAAAAAa aeeeeeeeeeeeeeeeeeeeeee                 ddddddddddddd", "ee", 60);
	ft_substr("char const *s", 5, 8);
	ft_strrchr("const char *s", '5');
	ft_strjoin("char const *s1", "char const *s2");
	ft_strtrim("     char const *str     ", " cr");
	ft_strmapi("char const *s", &changeC);
	ft_striteri("char *s", &changeString);
	ft_strncmp("const char *s1", "const char *s2", 14);

	ft_putchar_fd('\n', 1);
	ft_putstr_fd("Helo >Orld\n", 1);
	ft_putendl_fd("Hello world with new line", 1);
	ft_putnbr_fd(0x7FFFFFFF, 1);

	ft_tolower('O');
	ft_toupper('s');

	/*t_list	*ft_lstnew(void *content);
	t_list	*ft_lstlast(t_list *lst);
	t_list	*ft_lstmap(t_list *lst, void *(*f)(void *), void (*del)(void *));
	ft_lstadd_front(t_list **lst, t_list *new);
	ft_lstadd_back(t_list **lst, t_list *new);
	ft_lstdelone(t_list *lst, void (*del)(void *));
	ft_lstclear(t_list **lst, void (*del)(void *));
	ft_lstiter(t_list *lst, void (*f)(void *));
	ft_lstsize(t_list *lst);*/

	ft_isalpha('j');
	ft_putnbr_fd(ft_atoi(" 4584He4"), 1);
	ft_putchar_fd('\n', 1);
	ft_putstr_fd(ft_itoa(-48512), 1);
	return (0);
}

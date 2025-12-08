apshell: main.c apsh_cd.c apsh_execute_pipeline.c apsh_exit.c apsh_handle_sig.c apsh_background.c apsh_export.c lru_history.c apsh_add_prompt.c
	gcc -Wall main.c apsh_cd.c apsh_execute_pipeline.c apsh_export.c apsh_exit.c apsh_background.c apsh_handle_sig.c lru_history.c apsh_add_prompt.c -o apshell

clean:
	rm -f apshell
	
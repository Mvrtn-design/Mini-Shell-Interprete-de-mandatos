# Mini-Shell-Interprete-de-mandatos
Se implementa que programa que actúa como intérprete de mandatos. El minishell interpreta y ejecuta mandatos leyéndolos de la entrada estándar. En definitiva, el interprete es capaz de:
- Ejecutar una secuencia de uno o varios mandatos separados por el carácter '|'.
- Permitir redirecciones:

  • Entrada: < fichero. Sólo puede realizarse sobre el primer mandato del pipe.

  • Salida: > fichero. Sólo puede realizarse sobre el último mandato del pipe.

  • Error: > & fichero. Sólo puede realizarse sobre el último mandato del pipe.

- Permitir la ejecución en background de la secuencia de mandatos si termina con el carácter `&'. Para ello, el minishell muestra el pid del proceso por el que espera entre corchetes, y no bloquearse por la ejecución de dicho mandato (es decir, no espera a mostrar el prompt a su terminación).

A grandes rasgos, el programa hace:
- Muestra en pantalla un prompt (los símbolos msh > seguidos de un espacio).
- Leer una línea del teclado.
- Analiza esa línea utilizando la librería parser .
- Ejecuta todos los mandatos de la línea a la vez creando varios procesos hijo y comunicando unos con otros con las tuberías que sean necesarias, y realizando las redirecciones que sean necesarias. En caso de que no se ejecute en background.
- Reconoce y ejecutar en foreground líneas con un solo mandato y 0 o más argumentos.

Teniendo en cuenta lo siguiente:
- Si la línea introducida no contiene ningún mandato o se ejecuta el mandato en background,se volverá a mostrar el prompt a la espera de una nueva línea.
- Se mostrará un mensaje de error cuando:
  . Si alguno de los mandatos a ejecutar no existe.
  . Si se produce algún error al abrir cualquiera de los ficheros de las redirecciones.
- Ni el minishell ni los procesos en background analizarán las señales desde teclado SIGINT (Ctrl + C) y SIGQUIT (Ctrl +/).

#include "megap_asm.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct { char *name; char *content; } FileEntry;

static char *dup_s(const char *s){ size_t n=strlen(s); char *p=(char*)malloc(n+1); if(p) memcpy(p,s,n+1); return p; }
static void normalize_token(char *s){ size_t i=0,j=strlen(s); while(i<j&&isspace((unsigned char)s[i]))i++; while(j>i&&(isspace((unsigned char)s[j-1])||s[j-1]==';'))j--; if(j>i&&(s[i]=='"'||s[i]=='\''))i++; if(j>i&&(s[j-1]=='"'||s[j-1]=='\''))j--; memmove(s,s+i,j-i); s[j-i]='\0'; }

static int file_exists(const char *p){ FILE *f=fopen(p,"rb"); if(!f) return 0; fclose(f); return 1; }

static void join_path(char *out, size_t cap, const char *dir, const char *name){
    size_t dlen=strlen(dir), nlen=strlen(name);
    size_t pos=0;
    if(cap==0) return;
    out[0]='\0';
    if(dlen>=cap) dlen=cap-1;
    memcpy(out,dir,dlen);
    pos=dlen;
    out[pos]='\0';
    if(pos+1<cap && pos>0 && out[pos-1]!='/') out[pos++]='/';
    size_t rem=cap-pos-1;
    if(nlen>rem) nlen=rem;
    memcpy(out+pos,name,nlen);
    out[pos+nlen]='\0';
}

static int resolve_include(char *out, size_t cap, const char *asm_dir, const char *name){
    join_path(out,cap,asm_dir,name); if(file_exists(out)) return 1;
    char tmp[PATH_MAX]; join_path(tmp,sizeof(tmp),asm_dir,"includes"); join_path(out,cap,tmp,name); if(file_exists(out)) return 1;
    snprintf(out,cap,"%s",name); return file_exists(out);
}

static int load_include_recursive(const char *path, const char *asm_dir, FileEntry **files, size_t *count, size_t *cap){
    char *content=asm_read_file(path); if(!content) return 0;
    const char *base=strrchr(path,'/'); base=base?base+1:path;
    for(size_t i=0;i<*count;i++) if(!strcmp((*files)[i].name,base)){ free(content); return 1; }
    if(*count==*cap){ *cap=*cap?*cap*2:16; *files=(FileEntry*)realloc(*files,*cap*sizeof(FileEntry)); }
    (*files)[*count].name=dup_s(base); (*files)[*count].content=content; (*count)++;

    char *scan=dup_s(content); char *save=NULL; char *line=strtok_r(scan,"\n",&save);
    while(line){
        char tmp[1024]; snprintf(tmp,sizeof(tmp),"%s",line);
        char *comment=strstr(tmp,"//"); if(comment) *comment='\0'; comment=strchr(tmp,';'); if(comment) *comment='\0';
        char mn[64]={0}; if(sscanf(tmp,"%63s",mn)==1){ for(char *p=mn;*p;p++) *p=(char)toupper((unsigned char)*p); if(!strcmp(mn,"INCLUDE")){ char *rest=strchr(tmp,' '); if(rest){ while(*rest==' ') rest++; normalize_token(rest); char full[PATH_MAX]; if(resolve_include(full,sizeof(full),asm_dir,rest)) load_include_recursive(full,asm_dir,files,count,cap); } } }
        line=strtok_r(NULL,"\n",&save);
    }
    free(scan); return 1;
}

int main(int argc,char **argv){
    if(argc<2){ fprintf(stderr,"Uso: %s <archivo.asm> [--lst] [--out <archivo.hex>] [--lst-out <archivo.lst>]\n",argv[0]); return 1; }
    const char *asm_path=NULL,*out_path=NULL,*lst_path=NULL; int write_lst=0;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"--lst")) write_lst=1; else if(!strcmp(argv[i],"--out")&&i+1<argc) out_path=argv[++i];
        else if(!strcmp(argv[i],"--lst-out")&&i+1<argc){ lst_path=argv[++i]; write_lst=1; }
        else if(argv[i][0]=='-'){ fprintf(stderr,"Argumento no reconocido: %s\n",argv[i]); return 1; }
        else if(!asm_path) asm_path=argv[i]; else { fprintf(stderr,"Solo se permite un archivo .asm\n"); return 1; }
    }
    char *source=asm_read_file(asm_path); if(!source){ fprintf(stderr,"No se pudo abrir: %s\n",asm_path); return 1; }

    char asm_dir[PATH_MAX]="."; const char *slash=strrchr(asm_path,'/');
    if(slash){ snprintf(asm_dir,sizeof(asm_dir),"%.*s",(int)(slash-asm_path),asm_path); }

    FileEntry *inc_files=NULL; size_t inc_count=0,inc_cap=0;
    char defs[PATH_MAX]; if(resolve_include(defs,sizeof(defs),asm_dir,"Megaprocessor_defs.asm")) load_include_recursive(defs,asm_dir,&inc_files,&inc_count,&inc_cap);

    char *scan=dup_s(source),*save=NULL,*line=strtok_r(scan,"\n",&save);
    while(line){ char tmp[1024]; snprintf(tmp,sizeof(tmp),"%s",line); char *comment=strstr(tmp,"//"); if(comment)*comment='\0'; comment=strchr(tmp,';'); if(comment)*comment='\0'; char mn[64]={0};
        if(sscanf(tmp,"%63s",mn)==1){ for(char *p=mn;*p;p++) *p=(char)toupper((unsigned char)*p); if(!strcmp(mn,"INCLUDE")){ char *rest=strchr(tmp,' '); if(rest){ while(*rest==' ')rest++; normalize_token(rest); char full[PATH_MAX]; if(resolve_include(full,sizeof(full),asm_dir,rest)) load_include_recursive(full,asm_dir,&inc_files,&inc_count,&inc_cap); } }}
        line=strtok_r(NULL,"\n",&save);
    }
    free(scan);

    IncludeEntry *entries=inc_count?(IncludeEntry*)malloc(sizeof(IncludeEntry)*inc_count):NULL;
    for(size_t i=0;i<inc_count;i++){ entries[i].name_upper=inc_files[i].name; entries[i].content=inc_files[i].content; }

    Assembler as; assembler_init(&as); assembler_set_include_files(&as,entries,inc_count);
    char *hex=NULL; if(!assembler_assemble(&as,source,&hex)){ fprintf(stderr,"ERROR: %s\n",as.error[0]?as.error:"ensamblado falló"); return 2; }

    char out_auto[PATH_MAX]; if(!out_path){ snprintf(out_auto,sizeof(out_auto),"%s",asm_path); char *d=strrchr(out_auto,'.'); if(d) strcpy(d,".hex"); else strcat(out_auto,".hex"); out_path=out_auto; }
    if(!asm_write_file(out_path,hex)){ fprintf(stderr,"No se pudo escribir %s\n",out_path); return 1; }
    if(write_lst){ char lst_auto[PATH_MAX]; if(!lst_path){ snprintf(lst_auto,sizeof(lst_auto),"%s",asm_path); char *d=strrchr(lst_auto,'.'); if(d) strcpy(d,".lst"); else strcat(lst_auto,".lst"); lst_path=lst_auto; } if(!asm_write_file(lst_path,assembler_get_listing(&as))){ fprintf(stderr,"No se pudo escribir %s\n",lst_path); return 1; }}
    printf("HEX generado: %s\n",out_path);

    assembler_free(&as); free(source); free(hex); for(size_t i=0;i<inc_count;i++){ free(inc_files[i].name); free(inc_files[i].content);} free(inc_files); free(entries);
    return 0;
}

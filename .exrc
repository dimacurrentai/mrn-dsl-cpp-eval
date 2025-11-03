if &cp | set nocp | endif
let s:cpo_save=&cpo
set cpo&vim
map! <D-v> *
xmap gx <Plug>NetrwBrowseXVis
nmap gx <Plug>NetrwBrowseX
xnoremap <silent> <Plug>NetrwBrowseXVis :call netrw#BrowseXVis()
nnoremap <silent> <Plug>NetrwBrowseX :call netrw#BrowseX(netrw#GX(),netrw#CheckIfRemote(netrw#GX()))
map <F12> :YcmCompleter FixIt
map <F9> :w :mak test
map <F8> :mak clean
map <F7> :w :mak
map <F3> :cp
map <F4> :cn
vmap <BS> "-d
vmap <D-x> "*d
vmap <D-c> "*y
vmap <D-v> "-d"*P
nmap <D-v> "*P
let &cpo=s:cpo_save
unlet s:cpo_save
set autoindent
set background=dark
set backspace=indent,eol,start
set completeopt=menu,longest,preview
set expandtab
set fileencodings=ucs-bom,utf-8,default,latin1
set fileformats=unix,dos,mac
set helplang=en
set hlsearch
set matchpairs=(:),{:},[:],<:>
set modelines=0
set ruler
set runtimepath=~/.vim,/usr/share/vim/vimfiles,/usr/share/vim/vim91,/usr/share/vim/vimfiles/after,~/.vim/after,~/.vim/pack/plugins/opt/YouCompleteMe
set shiftwidth=2
set showmatch
set smartindent
set softtabstop=2
set tabstop=2
set wildignore=*.o,*~,*.pyc
set window=0
" vim: set ft=vim :

function __comp_debug()
{
	local cur="${COMP_WORDS[COMP_CWORD]}"
	local prev="${COMP_WORDS[COMP_CWORD-1]}"
	
	case $prev in
		--level | -L)
			local options="verbose debug info warn error fatal"
			COMPREPLY=($(compgen -W "$options" -- ${cur#=}))
			return 0
			;;
	esac
	
	case $cur in
		--*)
			local options="level list help check force recursive interface"
			COMPREPLY=($(compgen -P "--" -W "$options" -- ${cur#--}))
			;;
		-*)
			local options="l r h c f F i"
			COMPREPLY=($(compgen -P "-" -W "$options" -- ${cur#-}))
			;;
		*)
			local modules=`debug --no-tag --list`
			COMPREPLY=($(compgen -W "$modules" -- ${cur}))
			;;
	esac
}

complete -F __comp_debug debug

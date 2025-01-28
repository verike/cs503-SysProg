#!/bin/sh

set -eu

# Display usage information
usage() {
    cat <<EOF
Usage: $0 <command> <command-argument>

Commands:
  list         List all assignments
  pull <name>  Pull the starter code and directions
  launch       Set up the local .vscode folder with necessary configuration
EOF
}

get_auth_header() {
    if [ -n "${GITHUB_API_TOKEN:-}" ]; then
        echo "-H 'Authorization: token $GITHUB_API_TOKEN'"
    else
        echo ""
    fi
}

# Command: greet
list_command() {
    api_url="https://api.github.com/repos/drexel-systems/SysProg-Class/contents/assignments"
    
    echo "Fetching assignements list curl -s $(get_auth_header) $api_url ..."

    curl -s $(get_auth_header) "$api_url" | jq -r '.[] | select(.type == "dir") | "\(.url) \(.name)"' | while read -r url name; do
        echo "    $name"
    done
}

launch_command() {
    vscode_tree_api="https://api.github.com/repos/drexel-systems/sysprog-overview/git/trees/main?recursive=1"
    vscode_folder=".vscode"

    echo "Setting up $vscode_folder folder from $vscode_tree_api..."

    # Create the .vscode folder if it doesn't exist
    mkdir -p "$vscode_folder"

    # Fetch the tree and filter for files in the .vscode directory
    curl -s $(get_auth_header) "$vscode_tree_api" | jq -r '.tree[] | select(.path | startswith(".vscode/")) | select(.type == "blob") | "\(.path) \(.url)"' | while read -r path blob_url; do
        # Remove ".vscode/" from the download path
        local_path="${path#.vscode/}"

        echo "    Downloading file: $local_path"

        # Fetch the file content from the blob API
        content=$(curl -s $(get_auth_header) "$blob_url" | jq -r '.content')
        encoding=$(curl -s $(get_auth_header) "$blob_url" | jq -r '.encoding')

        echo "$content" | base64 -d > "$vscode_folder/$local_path" # Decode and save the file content
    done

    echo "Setup completed for $vscode_folder"
}

pull_command() {
    if [ $# -lt 1 ]; then
        echo "Error: missing assignment name"
        echo ""
        usage
        exit 1
    fi

    assignment_name="$1"
    tree_api_url="https://api.github.com/repos/drexel-systems/SysProg-Class/git/trees/main?recursive=1"

    # Check if the assignment folder already exists locally
    if [ -d "$assignment_name" ]; then
        echo "Error: The folder '$assignment_name' already exists locally."
        echo "Please rename or move the folder if you want to pull a fresh copy."
        exit 1
    fi

    echo "Fetching assignment $assignment_name from $tree_api_url..."

    # Fetch the tree and filter for files in the assignment directory
    curl -s $(get_auth_header) "$tree_api_url" | jq -r --arg assignment "assignments/$assignment_name/" '
        .tree[] | select(.path | startswith($assignment)) | select(.type == "blob") | "\(.path) \(.url)"
    ' | while read -r path blob_url; do
        # Remove "assignments/" from the download path
        local_path="${path#assignments/}"

        echo "    Downloading file: $local_path"

        # Fetch the file content from the blob API
        content=$(curl -s $(get_auth_header) "$blob_url" | jq -r '.content')
        encoding=$(curl -s $(get_auth_header) "$blob_url" | jq -r '.encoding')

        if [ "$encoding" = "base64" ]; then
            mkdir -p "$(dirname "$local_path")" # Ensure the target directory exists
            echo "$content" | base64 -d > "$local_path" # Decode and save the file content
        else
            echo "    Warning: Unexpected encoding ($encoding) for file $local_path; contents: $content"
        fi
    done

    echo "Download completed for assignment: $assignment_name"
    echo
    echo "========================================================================================"
    echo "* Review $assignment_name/readme.md for instructions on how to complete the assignment "
    echo "*"
    echo "* Happy coding! For assitance, please reach out via discord:                           "
    echo "*"
    echo "*            https://discord.com/channels/798568353689501758/1324393213850681394       "
    echo "*"
    echo "========================================================================================"
}

check_dependencies() {
    for cmd in jq curl; do
        if ! command -v "$cmd" >/dev/null 2>&1; then
            echo "Error: '$cmd' is not installed. Run 'sudo apt update && sudo apt install jq curl -y'."
            exit 1
        fi
    done
}

# Parse the command
if [ $# -lt 1 ]; then
    usage
    exit 1
fi

check_dependencies

command="$1"
shift

case "$command" in
    list)
        list_command "$@"
        ;;
    pull)
        pull_command "$@"
        ;;
    launch)
        launch_command "$@"
        ;;
    help|*)
        usage
        ;;
esac

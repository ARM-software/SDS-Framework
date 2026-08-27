#!/usr/bin/env bash
# Version: 3.1
# Date: 2024-04-17
# This bash script generates the documentation
#

set -o pipefail

# Set version of gen pack library
# For available versions see https://github.com/Open-CMSIS-Pack/gen-pack/tags.
# Use the tag name without the prefix "v", e.g., 0.7.0
REQUIRED_GEN_PACK_LIB="0.11.3"

DIRNAME=$(dirname "$(readlink -f "$0")")

# Set GEN_PACK_LIB_PATH to use a specific gen-pack library root
# ... instead of bootstrap based on REQUIRED_GEN_PACK_LIB
if [[ -n "${GEN_PACK_LIB_PATH}" ]] && [[ -f "${GEN_PACK_LIB_PATH}/gen-pack" ]]; then
  . "${GEN_PACK_LIB_PATH}/gen-pack"
else
  . <(curl -sL "https://raw.githubusercontent.com/Open-CMSIS-Pack/gen-pack/main/bootstrap")
fi

UTILITY_MKDOCS=$(find_utility "mkdocs")
report_utility "mkdocs" "${UTILITY_MKDOCS}" $? || (echo_log "Hint: Run pip install mkdocs"; exit 1)
find_linkchecker

echo_log "Generating documentation ..."

"${UTILITY_MKDOCS}" build -f "${DIRNAME}/../mkdocs.yml"  || exit

check_links --timeout 120 "${DIRNAME}/../docs/index.html" "${DIRNAME}"

exit 0

#!/bin/sh
echo "These links are for allowing legacy tutorial folder structures to work"
echo "Installing..."
ln -s /LF/Bulk/LanguagePack_en/Tutorials LF/Base/Tutorials
echo "To remove compatibility links type:"
echo "   rm LF/Base/Tutorials"
echo "These links are for allowing legacy movies to display errors"
echo "Installing..."
ln -s /LF/Base/L3B/Art LF/Base/L3B_Art
ln -s /LF/Base/L3B/Audio LF/Base/L3B_Audio
echo "To remove compatibility links type:"
echo "   rm LF/Base/L3B_Art LF/Base/L3B_Audio"

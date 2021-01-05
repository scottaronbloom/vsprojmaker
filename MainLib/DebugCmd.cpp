// The MIT License( MIT )
//
// Copyright( c ) 2020-2021 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "DebugCmd.h"
#include <QApplication>
#include <QMessageBox>
#include <QFile>

QDataStream & operator<<( QDataStream & stream, const NVSProjectMaker::SDebugCmd & value )
{
    stream << value.fSourceDir
        << value.fName
        << value.fCmd
        << value.fArgs
        << value.fWorkDir
        << value.fEnvVars
        ;
    return stream;
}

QDataStream & operator>>( QDataStream & stream, NVSProjectMaker::SDebugCmd & value )
{
    stream >> value.fSourceDir;
    stream >> value.fName;
    stream >> value.fCmd;
    stream >> value.fArgs;
    stream >> value.fWorkDir;
    stream >> value.fEnvVars;

    return stream;
}

QDataStream & operator<<( QDataStream & stream, const QList< NVSProjectMaker::SDebugCmd > & value )
{
    stream << value.count();
    for ( auto ii = value.constBegin(); ii != value.constEnd(); ++ii )
    {
        stream << *ii;
    }
    return stream;
}

QDataStream & operator>>( QDataStream & stream, QList< NVSProjectMaker::SDebugCmd > & value )
{
    int count;
    stream >> count;

    for ( int ii = 0; ii < count; ++ii )
    {
        NVSProjectMaker::SDebugCmd curr;
        stream >> curr;
        value.push_back( curr );
    }
    return stream;
}

namespace NVSProjectMaker
{
    QString SDebugCmd::getEnvVars() const
    {
        QStringList tmp;
        bool inParen = false;
        QString curr;
        for ( int ii = 0; ii < fEnvVars.length(); ++ii )
        {
            auto currChar = fEnvVars[ ii ];
            if ( inParen )
            {
                if ( currChar == '}' )
                {
                    inParen = false;
                    curr = curr.trimmed();
                    tmp << curr;
                    curr.clear();
                }
                else
                    curr += currChar;
            }
            else
                inParen = currChar == '{';
        }
        if ( !curr.isEmpty() )
            tmp << curr;
        auto retVal = tmp.join( '\n' );
        return retVal;
    }

    QString SDebugCmd::getProjectName() const
    {
        QString retVal = fName;
        retVal.replace( "/", "_" );
        retVal = retVal.replace( "\\", "_" );
        retVal = retVal.replace( " ", "_" );
        return retVal;
    }

    QString SDebugCmd::getCmd() const
    {
        QString retVal = fCmd;
        retVal.replace( "\\", "/" );
        return retVal;
    }

    void SDebugCmd::cleanUp( const QMap< QString, QString > & map )
    {
        for ( auto && ii = map.cbegin(); ii != map.cend(); ++ii )
        {
            auto key = "<" + ii.key() + ">";
            auto value = ii.value();
            fCmd = fCmd.replace( key, value );
            fArgs = fArgs.replace( key, value );
            fWorkDir = fWorkDir.replace( key, value );
            fEnvVars = fEnvVars.replace( key, value );
        }
    }

}
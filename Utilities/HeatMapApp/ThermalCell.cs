using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace HeatMapApp
{
    class ThermalCell : INotifyPropertyChanged
    {
        public float CellValue
        {
            get
            {
                return cellValue;
            }
            set
            {
                if (value != cellValue)
                {
                    cellValue = value;
                    OnPropertyChanged();
                }
            }
        }
        private float cellValue = 0;

        public event PropertyChangedEventHandler PropertyChanged;

        private void OnPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}

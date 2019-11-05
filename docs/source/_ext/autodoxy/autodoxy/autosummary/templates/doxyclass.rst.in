{{ name }}
{{ underline }}

.. autodoxyclass:: {{ fullname }}
   :members:

   {% if methods %}
   .. rubric:: Methods

   .. autodoxysummary::
   {% for item in methods %}
      ~{{ fullname }}::{{ item }}
   {%- endfor %}
   {% endif %}

   {% if enums %}
   {% for enum in enums %}
   .. autodoxyenum:: {{ enum }}
   {% endfor %}
   {% endif %}